#include "byte_stream.hh"
#include <assert.h>
#include <iostream>
using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), buf_(capacity_ << 1, ' '), st_(0), cur_sz_(0) {}

void Writer::push( string data ){
  uint64_t incr_ = min(data.size(), capacity_ - cur_sz_);
  uint64_t nxt_sz_ = incr_ + cur_sz_;
  for(uint64_t i = 0, j = (st_+cur_sz_)%capacity_, k = st_+cur_sz_; i < incr_; i++, j = (j==capacity_-1 ? 0 : j+1), k++){
    buf_[j] = data[i];
  }
  for(uint64_t k = max(capacity_, st_ + cur_sz_), i = k - st_ - cur_sz_; k < st_ + nxt_sz_; k++, i++){
    buf_[k] = data[i];
  }
  // cout << buf_ << endl;
  cur_sz_ = nxt_sz_;
  nwrite_ += incr_;
}

void Writer::close(){
  // buf_[cur_sz_ + st_] = EOF; // maybe overflow
  is_closed_ = true;
}

bool Writer::is_closed() const{
  return is_closed_;
}

uint64_t Writer::available_capacity() const{
  return capacity_ - cur_sz_;
}

uint64_t Writer::bytes_pushed() const{
  return nwrite_;
}

string_view Reader::peek() const{
  return string_view(buf_.data() + st_, cur_sz_);
}

void Reader::pop( uint64_t len ){
  uint64_t npop = min(len, cur_sz_);
  st_ = (st_ + npop) % capacity_;
  cur_sz_ -= npop;
  nread_ += npop;
}

bool Reader::is_finished() const{
  return is_closed_ && (cur_sz_ == 0);
}

uint64_t Reader::bytes_buffered() const{
  return cur_sz_;
}

uint64_t Reader::bytes_popped() const{
  return nread_;
}

