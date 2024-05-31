#include "tcp_receiver.hh"
#include <cstdint>
#include <iostream>

using namespace std;
// static constexpr uint16_t UINT16_MAX = 65535;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if ( message.RST ) {
    reassembler_.reader().set_error();
    return;
  }
  if ( connectStart_ == false ) { // 判断连接是否开始
    if ( !message.SYN )           // 如果不是连接开始，则丢弃该包
      return;
    connectStart_ = true;
    zero_point_ = message.seqno; // 拿到初始序列号seq，方便后期32位转64时，记录初试的32位序列
    reassembler_.insert( 0, message.payload, message.FIN );
  } else {
    checkpoint_ = reassembler_.writer().bytes_pushed() + 1; // 上一个数据包放在了哪儿
    uint64_t seq_index
      = message.seqno.unwrap( zero_point_, checkpoint_ ); // 32位转64位,zero_point时初试的syn的序列号
    reassembler_.insert( seq_index - 1,
                         message.payload,
                         message.FIN ); // 减一是因为send()数据是从1开始的，但是reassembler是从0开始插入数据的
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage msg {};
  if ( reassembler_.reader().has_error() || reassembler_.writer().has_error() ) { // 检测到rst，则关闭连接
    msg.RST = true;
    return msg;
  }
  auto const window_sz = reassembler_.writer().available_capacity(); // 窗口大小设置位缓冲区大小
  msg.window_size = window_sz < UINT16_MAX ? window_sz : UINT16_MAX; // 最大可以设置为多大
  uint64_t seq_index
    = reassembler_.writer().bytes_pushed() + 1 + this->writer().is_closed(); // 剔除掉头部又syn和fin的标签
  if ( connectStart_ )
    msg.ackno = Wrap32::wrap( seq_index, zero_point_ ); // 发送ack

  return msg;
}
