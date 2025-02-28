#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const{
  if(seg_que_.empty())
    return 0;
  const Wrap32 &bck = seg_que_.back().seqno, &fnt = seg_que_.front().seqno;
  return bck.unwrap(isn_, sent_seq_) + seg_que_.back().sequence_length() - fnt.unwrap(isn_, ack_seq_);
}

uint64_t TCPSender::consecutive_retransmissions() const{
  return con_retrans_;
}

void TCPSender::push( const TransmitFunction& transmit ){
  if(FIN_sent_){
    return;
  }

  if(window_sz == 0){
    if(sent_seq_ == ack_seq_){
      std::string payload;
      read(reader(), 1, payload);
      TCPSenderMessage mes = make_empty_message();
      mes.payload = std::move(payload);
      if(reader().is_finished() && mes.sequence_length() == 0){
        mes.FIN = true;
        FIN_sent_ = true;
      }
      transmit(mes);
      sent_seq_ += 1;
      seg_que_.push(std::move(mes));
    }
    return;
  }

  // if(sent_seq_ == 0){
  //   TCPSenderMessage mes = make_empty_message();
  //   string payload;
  //   read(reader(), min(TCPConfig::MAX_PAYLOAD_SIZE, window_sz - 1ul), payload);
  //   mes.SYN = true;
  //   mes.FIN = (FIN_sent_ |= reader().is_finished());
  //   mes.payload = true;
  //   transmit(mes);
  //   seg_que_.push(std::move(mes));
  //   sent_seq_ = mes.sequence_length();
  // }


  while(sent_seq_ - ack_seq_ < window_sz
        && (reader().bytes_buffered() || (reader().is_finished() && !FIN_sent_) || sent_seq_ == 0)){
    TCPSenderMessage mes = make_empty_message();
    if(sent_seq_ == 0){
      mes.SYN = 1;
      sent_seq_ = 1;
    }

    std::string payload;
    read(reader(), min(TCPConfig::MAX_PAYLOAD_SIZE, window_sz - sent_seq_ + ack_seq_), payload);
    mes.payload = std::move(payload);
    if(reader().is_finished() && sent_seq_ - ack_seq_ + mes.sequence_length() < window_sz){
      mes.FIN = true;
      FIN_sent_ = true;
    }
    transmit(mes);
    sent_seq_ += mes.payload.size() + mes.FIN;
    seg_que_.push(std::move(mes));
  }
}

TCPSenderMessage TCPSender::make_empty_message() const{
  return { .seqno = Wrap32::wrap(sent_seq_, isn_), .RST = input_.has_error()};
}

void TCPSender::receive( const TCPReceiverMessage& msg ){
  //todo: deal with RST, what dose window_sz with size 0 mean
  if(msg.ackno.has_value() && msg.ackno.value().unwrap(isn_, ack_seq_) > sent_seq_){
    return;
  }
  if(msg.RST){
    input_.set_error();
  }
  window_sz = msg.window_size;
  if(msg.ackno.has_value()){
    ack_seq_ = msg.ackno.value().unwrap(isn_, ack_seq_);
  }
  if(seg_que_.empty()){
    return;
  }

  uint64_t end_of_fnt = seg_que_.front().seqno.unwrap(isn_, ack_seq_) + seg_que_.front().sequence_length();
  if(ack_seq_ >= end_of_fnt){
    con_retrans_ = 0;
    cur_RTO_ms_ = initial_RTO_ms_;
    retrans_timer = 0;
    seg_que_.pop();
  }

  while(!seg_que_.empty() && ack_seq_ >= seg_que_.front().seqno.unwrap(isn_, ack_seq_) + seg_que_.front().sequence_length()){
    seg_que_.pop();
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit ){
  if(seg_que_.empty()) return;
  retrans_timer += ms_since_last_tick;
  
  if(retrans_timer >= cur_RTO_ms_){
    // two kind of implementing here: resend all or only one
    transmit(seg_que_.front());

    con_retrans_++;
    retrans_timer = 0; //notice : 0 or minus cur_RTO_ms_
    if(window_sz)
      cur_RTO_ms_ = cur_RTO_ms_ * 2;
  }
}
