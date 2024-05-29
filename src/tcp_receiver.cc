#include "tcp_receiver.hh"

using namespace std;


void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if(message.SYN == true) {
    lastpoint = w_.unwrap(message.seqno,0);
    reassembler_.insert(0,message.payload,false);
  }
  if(message.FIN == true) {
    lastpoint = w_.unwrap(message.seqno, lastpoint);
    reassembler_.insert(lastpoint, message.payload, true);
    lastpoint = 0;
  }
  if(message.RST == true) {
    
    lastpoint = 0;
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  
}
