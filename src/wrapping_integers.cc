#include "wrapping_integers.hh"
#include "../util/tcp_config.hh"
#include <iostream>
#include <tcp_config.hh>

using namespace std;
uint64_t max_int = 4294967296;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point ) // 要发送出去的数据
{
  return Wrap32 { static_cast<uint32_t>( n ) + zero_point.raw_value_ };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const // 接收到的数据
{
  uint32_t seqno = this->raw_value_ - zero_point.raw_value_;
  uint64_t offset = static_cast<uint64_t>( seqno );
  uint64_t next = ( checkpoint / max_int ) * max_int + offset;
  uint64_t left = next - max_int;
  uint64_t right = next + max_int;
  if ( next > checkpoint ) {
    if ( next - checkpoint < checkpoint - left || next < max_int ) {
      return next;
    } else {
      return left;
    }
  } else {
    if ( right - checkpoint > checkpoint - next ) {
      return next;
    } else {
      return right;
    }
  }
}
