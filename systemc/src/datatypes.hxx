#ifndef __DATATYPES_HXX__
#define __DATATYPES_HXX__

#include <systemc>

namespace streebog_hw
{

struct gost_algctx
{
    sc_dt::sc_uint<512> h;
    sc_dt::sc_uint<512> N;
    sc_dt::sc_uint<512> sigma;
};

using gost_u512 = sc_dt::sc_biguint<512>;
using gost_u64  = sc_dt::sc_uint<64>;
using gost_u32  = sc_dt::sc_uint<32>;
using gost_u16  = sc_dt::sc_uint<16>;
using gost_u8   = sc_dt::sc_uint<8>;

using gost_i64  = sc_dt::sc_int<64>;
using gost_i32  = sc_dt::sc_int<32>;
using gost_i16  = sc_dt::sc_int<16>;
using gost_i8   = sc_dt::sc_int<8>;

};

#endif // __DATATYPES_HXX__
