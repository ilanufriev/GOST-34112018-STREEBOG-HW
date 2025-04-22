#include <array>
#include <cstddef>
#include <systemc>
#include <iostream>
#include <utils.hxx>
#include <stage.hxx>
#include <gost34112018.h>

namespace streebog_hw
{


/*
    Idk how fast it is, but it's for testing only
 */
void sc_uint512_to_c_array(unsigned char *dst, const u512 &number)
{
    for (int i = 0; i < 64; i++)
    {
        dst[i] = (number >> (i * 8)).to_int() & 0xFF;
    }
}

u512 c_array_to_sc_uint512(const unsigned char *from)
{
    u512 result = 0;
    for (int i = 0; i < 64; i++)
    {
        result <<= 8 * i;
        result |= from[i];
    }
    return result;
}

Stage::Stage(sc_core::sc_module_name const &name)
    : AUTONAME(block_i)
    , AUTONAME(block_size_i)
    , AUTONAME(sigma_i)
    , AUTONAME(n_i)
    , AUTONAME(h_i)
    , AUTONAME(ack_i)
    , AUTONAME(start_i)
    , AUTONAME(sel_i)
    , AUTONAME(sigma_nx_o)
    , AUTONAME(n_nx_o)
    , AUTONAME(h_nx_o)
    , AUTONAME(state_o)
{
    sigma_nx_o.bind(sigma_nx_s_);
    n_nx_o.bind(n_nx_s_);
    h_nx_o.bind(h_nx_s_);
    state_o.bind(state_s_);
}

void Stage::thread()
{
    GOST34112018_Context ctx;
    unsigned char block[64];
    unsigned char block_size;

    while (true)
    {
        switch (state_s_.read())
        {
            case State::CLEAR:
                {
                    WAIT_WHILE(start_i->read() != 0);
                    
                    sc_uint512_to_c_array(ctx.N, n_i->read());
                    sc_uint512_to_c_array(ctx.h, h_i->read());
                    sc_uint512_to_c_array(ctx.sigma, sigma_i->read());

                    break;
                }
            case State::BUSY:
                {
                    sc_uint512_to_c_array(block, block_i->read());
                    
                    block_size = block_size_i->read().to_uint() & 0xFF;

                    GOST34112018_HashBlock(block, block_size, &ctx);

                    sigma_nx_s_.write(c_array_to_sc_uint512(ctx.sigma));
                    h_nx_s_.write(c_array_to_sc_uint512(ctx.h));
                    n_nx_s_.write(c_array_to_sc_uint512(ctx.N));

                    state_s_.write(State::DONE);
                    break;
                }
            case State::DONE:
                {
                    WAIT_WHILE(ack_i->read() != 0);
                    break;
                }
        }
    }
}

void Stage::stage2()
{
    METHOD_NOT_IMPLEMENTED;
}

void Stage::stage3()
{
    METHOD_NOT_IMPLEMENTED;
}

};

