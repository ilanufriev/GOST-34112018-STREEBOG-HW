#ifndef __DATATYPES_HXX__
#define __DATATYPES_HXX__

#include <systemc>

namespace streebog_hw
{

using u512 = sc_dt::sc_biguint<512>;
using u64  = sc_dt::sc_uint<64>;
using u32  = sc_dt::sc_uint<32>;
using u16  = sc_dt::sc_uint<16>;
using u8   = sc_dt::sc_uint<8>;

using i64  = sc_dt::sc_int<64>;
using i32  = sc_dt::sc_int<32>;
using i16  = sc_dt::sc_int<16>;
using i8   = sc_dt::sc_int<8>;

template < typename T >
using out_export = sc_core::sc_export<sc_core::sc_signal_out_if<T>>;

template < typename T >
using in_port = sc_core::sc_port<sc_core::sc_signal_in_if<T>>;

};

#endif // __DATATYPES_HXX__
