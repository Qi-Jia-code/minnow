#include "reassembler.hh"
#include <algorithm>
using namespace std;

// Reassembler::Reassembler()
// : first_unacceptable_index_(0),
// first_unassembled_index_(0),
// buffer_(),
// flagbuf_(),
// endindex_(-1),
// init_flag_(1){}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  uint64_t remain_capacity = output_.writer().available_capacity();
  if(init_flag_ == true) {
    buffer_.resize(remain_capacity, 0);
    flagbuf_.resize(remain_capacity,0);
    init_flag_ = false;
  }

  if(is_last_substring == true) {
    endindex_ = first_index + data.size();
    first_unassembled_index_ = output_.writer().bytes_pushed();
    first_unacceptable_index_ = first_unassembled_index_ + remain_capacity;
    uint64_t str_begin;
    uint64_t str_end;

    if(!data.empty()) {
      if(first_index + data.size() > first_unassembled_index_ || 
        first_index < first_unacceptable_index_) {
          return;
      }else {
        str_begin = first_index;
        str_end = first_index + data.size() - 1;
        if(first_index < first_unassembled_index_) {
          str_begin = first_unassembled_index_ - first_index;
        }
        if(first_index + data.size() > first_unacceptable_index_) {
          str_end = first_unassembled_index_ - 1;
        }

        for(uint64_t i = str_begin; i < str_end; i++) {
          buffer_[i - first_unassembled_index_] = data[i - first_index];
          flagbuf_[i - first_unassembled_index_] = true;
        }
      }

    }

    string tmp;
    while(flagbuf_.front() == true) {
      tmp += buffer_.front();
      buffer_.pop_front();
      flagbuf_.pop_front();
      buffer_.push_back(0);
      flagbuf_.push_back(0);
    }

    output_.writer().push(tmp);
    if(output_.writer().bytes_pushed() == endindex_) {
      output_.writer().close();
    }
  }

}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return static_cast<uint64_t> (count(flagbuf_.begin(), flagbuf_.end(), true));
}
