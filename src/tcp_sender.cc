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

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.

  return outstanding_bytes_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return consecutive_retransmissions_num_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  bool can_push = true;
  while ( can_push ) { // 可能数据很大，窗口也足够，但是playload一次装不完，需要反复构造并发送
    TCPSenderMessage msg {};

    if ( isn_syn_ == false ) {
      isn_syn_ = true;
      msg.SYN = true;
      msg.seqno = isn_;
    }

    if ( input_.has_error() ) {
      msg.RST = true;
      isn_syn_ = false;
      msg.SYN = false;
      msg.FIN = false;
      return;
    }
    // if ( input_.writer().is_closed() ) {
    //   msg.FIN = true;
    // }
    msg.seqno = Wrap32::wrap( abs_seqno_, isn_ );
    uint64_t len = min( min( window_end_ + 1 - abs_seqno_, TCPConfig::MAX_PAYLOAD_SIZE ),
                        input_.reader().bytes_buffered() ); // bytes_buffered：还有多少未读的

    // if ( window_end_ + 1 == abs_seqno_
    //      && input_.reader().bytes_buffered() > 0 ) { // 如果窗口没空间，但是还有数据要发送，就发送一字节给对方
    //   len = 1;
    // }

    read( input_.reader(), len, msg.payload ); // 从缓冲区中读取字节
    // syn和fin也算窗口字节，窗口没空间时不能发送fin
    if ( input_.reader().is_finished() == true && !send_fin_
         && abs_seqno_ + len <= window_end_ ) { // 1.保证fin包是唯一的，不会生成序列号不同的fin包
                                                // 2.保证加上fin标记后的包也在窗口范围内
      msg.FIN = true;
      send_fin_ = true;
    }

    std::cout << "send pkg: " << abs_seqno_ << " " << msg.SYN << " " << msg.sequence_length() << " " << msg.FIN
              << std::endl;
    if ( msg.sequence_length() > 0 ) {
      outstanding_collection_.push_back( msg );
      abs_seqno_ += msg.sequence_length();
      std::cout << "push " << abs_seqno_ << " " << msg.sequence_length() << std::endl;
      outstanding_bytes_ += msg.sequence_length();
      transmit( msg );
      isStartTimer_ = true;
    } else {
      can_push = false; // 没window空间/没数据了
    }
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
  if ( !msg.ackno.has_value() ) { // 还没建立连接，只设置win_size即可
    window_end_ = msg.window_size > 0 ? msg.window_size - 1 : 0;
    return;
  }
  uint64_t ack_abs_sqn = msg.ackno.value().unwrap( isn_, abs_seqno_ );
  if ( ack_abs_sqn > abs_seqno_ ) { // 如果ack比还没发送的字节都大，说明这个ack包有问题，直接丢弃
    return;
  }
  std::cout << "win_end " << window_end_ << " " << ack_abs_sqn << " " << msg.window_size << std::endl;
  if ( ack_abs_sqn + msg.window_size - 1 > window_end_ ) {
    window_end_ = ack_abs_sqn + msg.window_size - 1;
  }
  primaltive_msg_window_size_ = msg.window_size;
  while ( !outstanding_collection_.empty() ) {
    const auto& node = outstanding_collection_.front();
    uint64_t abs_end = node.seqno.unwrap( isn_, abs_seqno_ ) + node.sequence_length();
    // std::cout << "pop front " << abs_end << " " << ack_abs_sqn << std::endl;
    if ( abs_end <= ack_abs_sqn ) {
      outstanding_bytes_ -= outstanding_collection_.front().sequence_length();
      outstanding_collection_.pop_front();
      if ( outstanding_bytes_ == 0 ) {
        // std::cout << "now" << std::endl;
        isStartTimer_ = false;
      } else {
        isStartTimer_ = true;
      }
      consecutive_retransmissions_num_ = 0;
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
      cur_RTO_ms_ = pow( 2, consecutive_retransmissions_num_ ) * initial_RTO_ms_;
    } else {
      cur_RTO_ms_ = initial_RTO_ms_;
    }
  }
}
