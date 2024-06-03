#include "reassembler.hh"

#include <iostream>
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if ( data.empty() && is_last_substring ) {
    output_.writer().close();
    return;
  }
  if ( is_last_substring ) {
    endindex_ = first_index + data.size();
  }
  uint64_t reas_capacity = output_.writer().available_capacity();
  // 边界条件
  if ( first_index >= byte_index_ + reas_capacity || ( first_index + data.size() <= byte_index_ ) ) {
    return;
  }

  // 截取左侧数据
  if ( first_index < byte_index_ ) {
    data = data.substr( byte_index_ - first_index );
    first_index = byte_index_;
  }

  auto last_it = map_.lower_bound( first_index );
  auto pre_it = last_it;
  bool has_pre = pre_it != map_.begin(), has_last = last_it != map_.end();
  if ( has_pre ) {
    pre_it--;
    // 如果当前数据包被包裹，就剔除当前包
    if ( pre_it->first + pre_it->second.size() >= first_index + data.size() ) {
      return;
    }
  }
  string temp_data = data;
  uint64_t temp_first_idx = first_index;

  // 判断左侧是否能连接并连接
  if ( has_pre && first_index <= pre_it->first + ( pre_it->second ).size() ) {
    // std::cerr<<first_index<<" "<<pre_it->first<<" "<<pre_it->second<<std::endl;
    string left_data = pre_it->second.substr( 0, first_index - pre_it->first );
    temp_data = left_data + temp_data;
    temp_first_idx = pre_it->first;
    buffer_size_ -= pre_it->second.size();
    map_.erase( pre_it->first );
  }
  // 判断是否有完全包裹的包，有的话就剔除掉
  if ( has_last ) {
    while ( last_it != map_.end()
            && last_it->first + last_it->second.size() <= temp_first_idx + temp_data.size() ) {
      auto delete_it = last_it;
      last_it++;
      buffer_size_ -= delete_it->second.size();
      map_.erase( delete_it );
    }
    if ( last_it == map_.end() )
      has_last = false;
  }

  // 判断右侧是否能连接并连接
  if ( has_last && first_index + data.size() >= last_it->first ) {
    // std::cerr<<first_index<<" "<<last_it->first<<" "<<last_it->second<<std::endl;
    string right_data = last_it->second.substr( first_index + data.size() - last_it->first );
    temp_data = temp_data + right_data;
    buffer_size_ -= last_it->second.size();
    map_.erase( last_it->first );
  }

  // 剪切多余的数据
  if ( temp_first_idx + temp_data.size() > byte_index_ + reas_capacity ) {
    temp_data = temp_data.substr( 0, byte_index_ + reas_capacity - temp_first_idx );
  }

  // 开始插入
  // 1.如果能与bytestream连接，直接push，不存入map_
  if ( temp_first_idx == byte_index_ ) {
    byte_index_ += temp_data.size();
    output_.writer().push( temp_data );
    // std::cerr<<"now:"<<byte_index_<<" "<<endindex_<<std::endl;
    if ( byte_index_ == endindex_ ) {
      output_.writer().close();
    }
    return;
  }

  // 2.插入map
  map_.emplace( temp_first_idx, temp_data );
  buffer_size_ += temp_data.size();
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return buffer_size_;
}