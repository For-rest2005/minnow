#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point ){
  return zero_point + static_cast<uint32_t>(n);
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const{
  static constexpr uint64_t bitmask = 0xffffffff00000000;
  static constexpr uint64_t bias = 0x80000000;
  uint64_t raw_abs = static_cast<uint64_t>(raw_value_ - zero_point.raw_value_);
  raw_abs += checkpoint & bitmask;
  if(raw_abs <= checkpoint && checkpoint - raw_abs >= bias && raw_abs + bias*2 > raw_abs){
    raw_abs += bias * 2;
  }
  else if(checkpoint < raw_abs && raw_abs - checkpoint >= bias && raw_abs - bias*2 < raw_abs){
    raw_abs -= bias * 2;
  }
  return raw_abs;
}
