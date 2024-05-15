#include "reassembler.hh"
<<<<<<< HEAD
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

=======
#include "byte_stream.hh"
#include "tcp_sender.hh"


using namespace std;

// Reassembler:Reassembler(ByteStream&& output)
// : output_(output),reas_capacity_(),byte_index(0){}
/// 插⼊⼀个新的⼦字符串以重新组装到 ByteStream 中

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if(is_last_substring == true) {
    if(first_index == byte_index_ + 1) {
      output_.writer().push(data);
      output_.writer().close();
    }else {
      umap.emplace(first_index, data);
      reas_remainder_ = reas_remainder_ + data.size();
    }
  }else if(first_index > byte_index_ + reas_capacity_ && is_last_substring == false) {
    return;
  }else if(first_index <= byte_index_ + reas_capacity_ && is_last_substring == false){
    uint64_t temp_index = first_index;
    if(temp_index == byte_index_ + 1) {
      output_.writer().push(data);
      byte_index_ = data.size() + byte_index_;
      temp_index = byte_index_ + 1;
      while(reas_remainder_ > 0) {
        if(umap.find(temp_index) != umap.end()) {
          output_.writer().push(umap[temp_index]);
          reas_remainder_ = reas_remainder_ - data.size();
          byte_index_ = data.size() + byte_index_;
          temp_index = byte_index_ + 1;
        }else {
          break;
        }
      }
    }else {
      umap.emplace(first_index, data);
      reas_remainder_ = reas_remainder_ - data.size();
    }
  }
  return;
>>>>>>> 679a202378d4eff417093352dcf31148c6e00989
}

// 重组器本⾝存储了多少字节？
uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
<<<<<<< HEAD
  return static_cast<uint64_t> (count(flagbuf_.begin(), flagbuf_.end(), true));
=======
  return reas_remainder_;
>>>>>>> 679a202378d4eff417093352dcf31148c6e00989
}


// private:
//   ByteStream output_; // the Reassembler writes to this ByteStream
//   unordered_map<uint64_t, string> umap; //key:first_index, value:data
//   uint64_t reas_capacity_;//重组启的大小
//   uint64_t reas_remainder_ = reas_capacity_;//重组启剩余的位置数量
//   uint64_t byte_index_ = 0; //维护字符串编号走到哪儿了