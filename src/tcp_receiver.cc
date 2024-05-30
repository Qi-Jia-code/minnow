#include "tcp_receiver.hh"
#include <iostream>

using namespace std;
#define UINT16_MAX (65535)

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if(connectStart_ == false) {
    if(message.SYN == true) {
      connectStart_ = true;
      last_ack_ = message.seqno;//把seqno的序号拿到，方便下次校准是不是这个连接的
      //uint64_t seqno1 = message.seqno.unwrap(last_ack_.value(),0);//把拿到的的数据包的seqno转成绝对数字，记录这次的位置，好方便下次定位下一个数据包的位置
      if(message.payload.size()+2 < reassembler_.writer().available_capacity() && message.FIN == true) {
        reassembler_.insert(0, message.payload, true);
        return;
      }else {
        reassembler_.insert(0, message.payload, false);
        //cout<<"now3 "<<reassembler_.writer().bytes_pushed()<<endl;
      }
    }
  }else {
    //std::cout<<"seqno " << message.seqno.unwrap(message.seqno, 0)<<std::endl;
    last_ack_ = message.seqno;
    if(message.FIN == true) {
      checkpoint_ = reassembler_.writer().bytes_pushed() + 1;
      uint64_t seqno1 = message.seqno.unwrap(last_ack_.value(), checkpoint_);
      first_index = seqno1 - 1;
      reassembler_.insert(first_index, message.payload, true);
      //std::cout<<"is_close"<<this->writer().is_closed()<<std::endl;
      return;
    }
    checkpoint_ = reassembler_.writer().bytes_pushed() + 1;
    uint64_t seqno1 = message.seqno.unwrap(last_ack_.value(), checkpoint_);
    first_index = seqno1 - 1;
    cout<<"now4 "<<reassembler_.writer().bytes_pushed()<<endl;
    reassembler_.insert(first_index, message.payload, false);
    cout<<"now2 "<<reassembler_.writer().bytes_pushed()<<endl;
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage msg {};
  auto const window_sz = reassembler_.writer().available_capacity();
  msg.window_size = window_sz < UINT16_MAX ? window_sz : UINT16_MAX;

  if(last_ack_.has_value()) {
    cout<<"now1 "<<reassembler_.writer().bytes_pushed()<<endl;
    uint64_t seqno1 = reassembler_.writer().bytes_pushed() + 1 + this->writer().is_closed(); //剔除掉头部又syn和fin的
    cout<<"seqno1 "<<seqno1<<endl;
    // cout<<"last_ack_.value()"<<last_ack_.value().<<endl;
    msg.ackno = Wrap32::wrap(seqno1, last_ack_.value());
  }
  return msg;
}
