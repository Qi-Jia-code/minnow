#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"

#include <cstdint>
#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) )
    , isn_( isn )
    , initial_RTO_ms_( initial_RTO_ms )
    , isn_syn_( 0 )
    , send_fin_( false )
    , cur_RTO_ms_( initial_RTO_ms )
    , isStartTimer_( 0 )
    , outstanding_bytes_( 0 )
    , window_end_( 0 )
    , abs_seqno_( 0 )
    , primaltive_msg_window_size_( 1 )
    , outstanding_collection_( {} )
    , consecutive_retransmissions_num_( 0 )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_; // 初试序列号
  uint64_t initial_RTO_ms_;
  bool isn_syn_ = false;
  bool send_fin_ = false;          // 是否已经发送过fin的包，如果发送过，就不再生成新的fin包
  int cur_RTO_ms_;                 // 当前重传的时间
  bool isStartTimer_ = false;      // 是否启动计时器
  uint64_t outstanding_bytes_ = 0; // 已发送未完成的字节数
  uint64_t window_end_ = 0;        // 可发送数据窗口的右侧，即窗口最右侧字节的绝对序号
  uint64_t abs_seqno_ = 0;         // 下一个未发送数据的第一个字节的绝对序列号
  uint16_t primaltive_msg_window_size_; // 保存最初拿到的msg的窗口大小，用来判断是否需要进行指数回退
  std::deque<TCPSenderMessage> outstanding_collection_; // 记录已发送但是未确认的数据包

  uint64_t consecutive_retransmissions_num_ = 0; // 重传次数
};
