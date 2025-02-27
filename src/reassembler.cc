#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring ){
  if(is_last_substring && last_str_idx == static_cast<uint64_t>(-1)){
    last_str_idx = first_index + data.size();
  }
  uint64_t last_index = first_index + data.size();
  if(first_index >= output_.reader().bytes_popped() + output_.capacity()){
    return;
  }

  uint64_t bytes_pushed_ = output_.writer().bytes_pushed();
  if(first_index <= bytes_pushed_ && last_index > bytes_pushed_){
    output_.writer().push(data.substr(bytes_pushed_ - first_index));

    while(!data_.empty() && data_.top().key_ <= output_.writer().bytes_pushed()){
      const valp &tmp = data_.top();
      first_index = tmp.key_, last_index = first_index + tmp.data_.size();
      bytes_pushed_ = output_.writer().bytes_pushed();
      if(first_index <= bytes_pushed_ && last_index > bytes_pushed_){
        output_.writer().push(tmp.data_.substr(bytes_pushed_ - first_index));
      }
      data_.pop();
    }
  }
  else if(last_index > output_.writer().bytes_pushed()){
    uint64_t buf_end_ = output_.reader().bytes_popped() + output_.capacity();
    if(data.size() + first_index > buf_end_){
      data.resize(buf_end_ - first_index);
    }
    data_.push({first_index, data});
  }

  if(output_.writer().bytes_pushed() == last_str_idx){
    output_.writer().close();
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const{
  uint64_t cnt = 0, cur = 0;
  std::priority_queue<valp> que = data_;
  while(!que.empty()){
    const valp& tmp = que.top();
    if(cur <= tmp.key_){
      cnt += tmp.data_.size();
      cur = tmp.key_ + tmp.data_.size();
    }
    else if(tmp.key_ + tmp.data_.size() > cur){
      cnt += tmp.key_ + tmp.data_.size() - cur;
      cur = tmp.key_ + tmp.data_.size();
    }
    que.pop();
  }
  return cnt;
}
