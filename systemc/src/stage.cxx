#include <cstdio>
#include <iterator>
#include <systemc>
#include <iostream>
#include <utils.hxx>
#include <stage.hxx>
#include <gost34112018.h>

#undef DEBUG_OUT_ENABLED
#define DEBUG_OUT_ENABLED 1

namespace streebog_hw
{

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

    SC_THREAD(thread);
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
                    DEBUG_OUT << "Stage is now at CLEAR\n";
                    WAIT_WHILE(start_i->read() == 0);

                    sc_uint512_to_bytes(ctx.N, 64, n_i->read());
                    sc_uint512_to_bytes(ctx.h, 64, h_i->read());
                    sc_uint512_to_bytes(ctx.sigma, 64, sigma_i->read());

                    advance_state(State::BUSY);
                    break;
                }
            case State::BUSY:
                {
                    DEBUG_OUT << "Stage is now at BUSY\n";
                    sc_uint512_to_bytes(block, 64, block_i->read());

                    block_size = block_size_i->read().to_uint() & 0xFF;

                    GOST34112018_HashBlock(block, block_size, &ctx);
                    
                    DEBUG_OUT << "ctx.N = " << bytes_to_hex_string(ctx.N, 64) << std::endl;
                    DEBUG_OUT << "ctx.h = " << bytes_to_hex_string(ctx.h, 64) << std::endl;
                    DEBUG_OUT << "ctx.sigma = " << bytes_to_hex_string(ctx.sigma, 64) << std::endl;

                    sigma_nx_s_.write(bytes_to_sc_uint512(ctx.sigma, 64));
                    h_nx_s_.write(bytes_to_sc_uint512(ctx.h, 64));
                    n_nx_s_.write(bytes_to_sc_uint512(ctx.N, 64));

                    advance_state(State::DONE);
                    break;
                }
            case State::DONE:
                {
                    DEBUG_OUT << "Stage is now at DONE\n";
                    WAIT_WHILE(ack_i->read() != 0);

                    advance_state(State::CLEAR);
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

void Stage::advance_state(State next_state)
{
    state_s_.write(next_state);
    WAIT_WHILE(state_s_.read() != next_state);
}

};

