#include "tcp_sender.hh"
#include "tcp_config.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <unistd.h>

using namespace std;

// TCPSender::TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
//   : input_( std::move( input ) )
//   , isn_( isn )
//   , initial_RTO_ms_( initial_RTO_ms )
//   , isn_syn_( 0 )
//   , isn_fin_( 0 )
//   , cur_RTO_ms_( initial_RTO_ms )
//   , isStartTimer_( 0 )
//   , outstanding_bytes_( 0 )
//   , receiving_msg_()
//   , abs_seqno_( 0 )
//   , primaltive_msg_window_size_( 1 )
//   , ready_collection_()
//   , outstanding_collection_()
//   , consecutive_retransmissions_num_( 0 )
// {}

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
  // Your code here.
  while ( outstanding_bytes_ < receiving_msg_.window_size ) {
    TCPSenderMessage msg {};
    if ( isn_syn_ == false ) {
      isn_syn_ = true;
      msg.SYN = true;
      msg.seqno = isn_;
    }
    msg.seqno = Wrap32::wrap( abs_seqno_, isn_ );

    uint64_t len = min( min( receiving_msg_.window_size - outstanding_bytes_, TCPConfig::MAX_PAYLOAD_SIZE ),
                        input_.reader().bytes_buffered() ); // bytes_buffered：还有多少未读的
    read( input_.reader(), len, msg.payload );              // 从缓冲区中读取字节

    if ( input_.reader().is_finished() == true
         && msg.sequence_length() + outstanding_bytes_ < receiving_msg_.window_size ) {
      if ( isn_fin_ == false ) {
        isn_fin_ = true;
        msg.FIN = true;
      }
    }

    if ( msg.sequence_length() == 0 ) {
      break;
    } else {
      ready_collection_.push_back( msg );
      outstanding_collection_.push_back( msg );
      outstanding_bytes_ += msg.sequence_length();
    }
    abs_seqno_ += msg.sequence_length();
    transmit( msg );
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  TCPSenderMessage msg {};
  msg.seqno = Wrap32::wrap( abs_seqno_, isn_ );
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  receiving_msg_ = msg;
  primaltive_msg_window_size_ = msg.window_size;
  if ( msg.ackno.has_value() ) {
    if ( msg.ackno.value().unwrap( isn_, abs_seqno_ ) > abs_seqno_ ) { // 确认的序列号比当前的序列号都大，则返回
      return;
    } else {
      while ( outstanding_bytes_ != 0
              && outstanding_collection_.front().seqno.unwrap( isn_, abs_seqno_ ) + receiving_msg_.window_size
                   < outstanding_collection_.size() ) {
        outstanding_bytes_ -= outstanding_collection_.front().sequence_length();
        outstanding_collection_.pop_front();

        if ( outstanding_bytes_ == 0 ) {
          isStartTimer_ = false;
        } else {
          isStartTimer_ = true;
        }
        consecutive_retransmissions_num_ = 0;
        cur_RTO_ms_ = initial_RTO_ms_;
      }
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
    ready_collection_.push_front( outstanding_collection_.front() );
    TCPSenderMessage msg { ready_collection_.front() };
    transmit( msg );
    ready_collection_.pop_front();
    consecutive_retransmissions_num_++;
    if ( primaltive_msg_window_size_ > 0 ) {
      cur_RTO_ms_ = pow( 2, consecutive_retransmissions_num_ ) * initial_RTO_ms_;
    } else {
      cur_RTO_ms_ = initial_RTO_ms_;
    }
  }
}
