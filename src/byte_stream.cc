#include "byte_stream.hh"
#include <iostream>


using namespace std;

ByteStream::ByteStream(uint64_t capacity) 
: capacity_( capacity ), close_ (0), q_(),writeCount_(0), readCount_(0){
}

bool Writer::is_closed() const
{ //Has the stream been closed?
  // Your code here.
  return close_;
}

void Writer::push( string data )
{
  // Your code here.
  for(auto&& i : data) {
    if(available_capacity() > 0) {
      q_.push(i);
      writeCount_++;
    }
  }
  return;
}

void Writer::close()
{
  // Your code here.
  close_ = true;
}

uint64_t Writer::available_capacity() const
{  
  // Your code here.
  uint64_t res = capacity_ - q_.size();
  return res;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return writeCount_;
}

bool Reader::is_finished() const
{
  // Your code here.
  // Is the stream finished (closed and fully popped)?
  if(q_.size() == 0 && close_) {
    return true;
  }
  return false;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return readCount_;
}

string_view Reader::peek() const
{
  // Your code here.
  return string_view(&q_.front(), 1);
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  if(len >= capacity_){
    for(uint64_t i = 0; i < capacity_; i++) {
      q_.pop();
      readCount_++;
    }
  }else {
    for(uint64_t i = 0; i < len; i++) {
      q_.pop();
      readCount_++;
    }
  }
  return;
}

uint64_t Reader::bytes_buffered() const
{ 
  // Your code here.
  return q_.size();
}