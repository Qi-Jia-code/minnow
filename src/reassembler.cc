#include "reassembler.hh"
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
}

// 重组器本⾝存储了多少字节？
uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return reas_remainder_;
}


// private:
//   ByteStream output_; // the Reassembler writes to this ByteStream
//   unordered_map<uint64_t, string> umap; //key:first_index, value:data
//   uint64_t reas_capacity_;//重组启的大小
//   uint64_t reas_remainder_ = reas_capacity_;//重组启剩余的位置数量
//   uint64_t byte_index_ = 0; //维护字符串编号走到哪儿了