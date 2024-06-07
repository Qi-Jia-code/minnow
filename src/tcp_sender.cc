#include "tcp_sender.hh"
#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <unistd.h>

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const // 返回已发送大那是未确认的字节数
{
  // Your code here.

  return outstanding_bytes_;
}

uint64_t TCPSender::consecutive_retransmissions() const // 重传次数
{
  // Your code here.
  return consecutive_retransmissions_num_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  bool can_push = true;
  while ( can_push ) { // while：可能数据很大，窗口也足够，但是playload一次装不完，需要反复构造并发送
    TCPSenderMessage msg {};

    if ( isn_syn_ == false ) { // 设置第一个头数据包
      isn_syn_ = true;
      msg.SYN = true;
      msg.seqno = isn_; // 领第一个seqno为isn_,isn_是随机数
    }

    if ( input_.has_error() ) { // 如果传输发生了错误，则返回rst,并发送
      msg.RST = true;
      msg.seqno = Wrap32::wrap( abs_seqno_, isn_ );
      isn_syn_ = false;
      msg.SYN = false;
      msg.FIN = false;
      transmit( msg );
      return;
    }

    msg.seqno = Wrap32::wrap( abs_seqno_, isn_ ); // 将64位绝对序列号转为32位网络序列号
    // syn也算窗口的空间(WINDOW_SIZE),占一字节
    uint64_t len = min(
      min( window_end_ + 1 - abs_seqno_ - static_cast<int>( msg.SYN ), TCPConfig::MAX_PAYLOAD_SIZE ),
      input_.reader()
        .bytes_buffered() ); // bytes_buffered：还有多少未读的，这句含义：选取可发送窗口的字节数，最大网络包长度，reader缓冲区中还有多少未读中最小的一个发送
    if ( need_detect_
         && input_.reader().bytes_buffered() > 0 ) { // 如果窗口没空间，但是还有数据要发送，就发送一字节给对方
      len = 1;
      window_end_++; // 窗口的右侧，如果要发送的话，则窗口必须++，否则会出现窗口的左侧（abs_seqno）大于右侧的情况
      need_detect_
        = false; // 发送某个序列号的探测包之后就不用再发送新的数据包了，如果还需发送，则重新发送上一个探测包
    }

    read( input_.reader(), len, msg.payload ); // 从缓冲区中读取字节
    // syn和fin也算窗口字节，窗口没空间时不能发送fin
    if ( input_.reader().is_finished() == true && !send_fin_
         && ( abs_seqno_ + len <= window_end_
              || need_detect_ ) ) { // 1.保证fin包是唯一的，不会生成序列号不同的fin包
                                    // 2.保证加上fin标记后的包也在窗口范围内或者是detect包
      msg.FIN = true;
      send_fin_ = true;
      if ( need_detect_ ) {
        window_end_++;
        need_detect_ = false;
      }
    }

    if ( msg.sequence_length() > 0 ) {
      outstanding_collection_.push_back( msg );
      abs_seqno_ += msg.sequence_length();
      outstanding_bytes_ += msg.sequence_length();
      transmit( msg );
      isStartTimer_ = true;
    }
    can_push = msg.sequence_length() >= TCPConfig::MAX_PAYLOAD_SIZE; // 是否发送完了
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  TCPSenderMessage msg {};
  msg.seqno = Wrap32::wrap( abs_seqno_, isn_ );
  msg.RST = input_.has_error();
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( msg.RST == true ) {
    input_.set_error();
    return;
  }

  if ( !msg.ackno.has_value() ) { // 还没建立连接，只设置win_size即可
    window_end_ = msg.window_size > 0 ? msg.window_size - 1 : 0;
    return;
  }

  uint64_t ack_abs_sqn = msg.ackno.value().unwrap( isn_, abs_seqno_ ); // 把32位网络序列号转换为64位绝对序列号
  if ( ack_abs_sqn > abs_seqno_ ) { // 如果ack比还没发送的字节都大，说明这个ack包有问题，直接丢弃
    return;
  }
  if ( ack_abs_sqn + msg.window_size
       > window_end_ + 1 ) { // 新来的数据包的序列加上他的窗口大小比sender窗口的右边都大，则更新sender窗口的有边界
    window_end_ = ack_abs_sqn + msg.window_size - 1;
  }
  if (
    msg.window_size == 0
    && window_end_ + 1
         == ack_abs_sqn ) { // 发送一个字节的探测，但是这个ack的msg可能因为网络延迟，已经不是最新的情况。最新的情况可能已经更新，window已经右移，不需要发送探测字节了。所以if后面的判断就是看是不是最新的情况
    need_detect_ = true;
  } else {
    need_detect_ = false;
  }
  primaltive_msg_window_size_ = msg.window_size;
  while ( !outstanding_collection_.empty() ) { // 收到的数据包中有数据，且缓冲区中也还有已发送但是为确认的数据
    const auto& node = outstanding_collection_.front();
    uint64_t abs_end = node.seqno.unwrap( isn_, abs_seqno_ ) + node.sequence_length();
    if ( abs_end <= ack_abs_sqn ) { // 受到的数据包的ack值要比缓冲区的头部序列要大时，才能将窗口向右滑动
      outstanding_bytes_ -= outstanding_collection_.front().sequence_length();
      outstanding_collection_.pop_front();
      if ( outstanding_bytes_ == 0 ) { // 如果缓冲区中已发送未确认的字节数为0，则关闭定时器
        isStartTimer_ = false;
      } else {
        isStartTimer_ = true;
      }
      consecutive_retransmissions_num_ = 0; // 受到数据了，说明网络状态有所好转，关闭指数回退
      cur_RTO_ms_ = initial_RTO_ms_;
    } else {
      break;
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  if ( isStartTimer_ == true ) {
    cur_RTO_ms_ -= ms_since_last_tick;
  }
  if ( cur_RTO_ms_ <= 0 ) { // 时间过了但是还没有受到ack
    TCPSenderMessage msg { outstanding_collection_.front() };
    transmit( msg );
    consecutive_retransmissions_num_++;
    if ( primaltive_msg_window_size_ > 0 ) {
      cur_RTO_ms_ = pow( 2, consecutive_retransmissions_num_ ) * initial_RTO_ms_; // 指数回退
    } else {
      cur_RTO_ms_ = initial_RTO_ms_;
    }
  }
}
