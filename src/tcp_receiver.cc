#include "tcp_receiver.hh"
#include "debug.hh"
#include <iostream>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message ){
  RST_ |= message.RST;

  if(message.SYN){
    zero_point_ = message.seqno;
    SYN_ = true;
    FIN_ = false;
  }

  reassembler_.insert(message.seqno.unwrap(zero_point_, reassembler_.writer().bytes_pushed()) - 1 + message.SYN, 
                      message.payload, message.FIN);
  reassembler_.set_error();

  FIN_ |= message.FIN;
}

TCPReceiverMessage TCPReceiver::send() const{
  uint64_t window_sz = reassembler_.writer().available_capacity();
  bool has_error = RST_ || reassembler_.has_error();
  window_sz = window_sz > 0xffff ? 0xffff : window_sz;

  if(!SYN_)
    return { nullopt, static_cast<uint16_t>(window_sz), has_error};

  bool fin_bit = FIN_ && reassembler_.writer().is_closed();
  return {
    Wrap32::wrap(reassembler_.writer().bytes_pushed() + 1 + fin_bit, zero_point_), 
    static_cast<uint16_t>(window_sz), has_error
  };
}
