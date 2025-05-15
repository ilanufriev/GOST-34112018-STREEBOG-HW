#include <cstdio>
#include <systemc>
#include <iostream>
#include <utils.hxx>
#include <stage.hxx>
#include <gost34112018.h>
#include <transformations.hxx>

#undef DEBUG_OUT_ENABLED
#define DEBUG_OUT_ENABLED 1

namespace streebog_hw
{

Stage::Stage(sc_core::sc_module_name const &name)
{
    SC_THREAD(thread);
}

void Stage::trace(sc_core::sc_trace_file *tf)
{
    sc_core::sc_trace(tf, sigma_nx_o, sigma_nx_o.name());
    sc_core::sc_trace(tf, n_nx_o, n_nx_o.name());
    sc_core::sc_trace(tf, h_nx_o, h_nx_o.name());
    sc_core::sc_trace(tf, state_o, state_o.name());
    sc_core::sc_trace(tf, g_n_m_o, g_n_m_o.name());
    sc_core::sc_trace(tf, g_n_n_o, g_n_n_o.name());
    sc_core::sc_trace(tf, g_n_h_o, g_n_h_o.name());
    sc_core::sc_trace(tf, g_n_trg_o, g_n_trg_o.name());
}

void Stage::thread()
{
    while (true)
    {
        switch (static_cast<State>(state_o.read().to_int()))
        {
            case State::CLEAR:
                {
                    DEBUG_OUT << "Stage is now at CLEAR\n";
 
                    h_ = n_ = sigma_ = 0;

                    h_nx_o.write(0);
                    n_nx_o.write(0);
                    sigma_nx_o.write(0);

                    g_n_m_o.write(0);
                    g_n_n_o.write(0);
                    g_n_h_o.write(0);
                    g_n_trg_o.write(0);

                    WAIT_WHILE_CLK(trg_i->read() == 0,
                                   clk_i->posedge_event());

                    advance_state(State::BUSY);
                    break;
                }
            case State::BUSY:
                {
                    DEBUG_OUT << "Stage is now at BUSY\n";
                    if (block_size_i->read() == BLOCK_SIZE)
                    {
                        stage2();
                    }
                    else
                    {
                        stage3();
                    }

                    advance_state(State::DONE);
                    break;
                }
            case State::DONE:
                {
                    DEBUG_OUT << "Stage is now at DONE\n";
                    WAIT_WHILE_CLK(trg_i->read() == 0,
                                   clk_i->posedge_event());

                    advance_state(State::CLEAR);
                    break;
                }
        }
        sc_core::wait(clk_i->posedge_event());
    }
}

u512 Stage::compute_gn(const u512 &h, const u512 &m, const u512&n)
{
    g_n_n_o.write(n);
    g_n_h_o.write(h);
    g_n_m_o.write(m);

    g_n_trg_o.write(1);

    sc_core::wait(clk_i->posedge_event());

    g_n_trg_o.write(0);

    WAIT_WHILE_CLK(g_n_state_i->read() != Gn::State::DONE,
                   clk_i->posedge_event());

    u512 result = g_n_result_i->read();

    g_n_trg_o.write(1);

    sc_core::wait(clk_i->posedge_event());

    g_n_trg_o.write(0); // This will reset Gn to CLEAR

    return result;
}

void Stage::stage2()
{
    DEBUG_OUT << "h_i = " << h_i->read().to_string(sc_dt::SC_HEX) << std::endl;
    DEBUG_OUT << "block_i = " << block_i->read().to_string(sc_dt::SC_HEX) << std::endl;
    DEBUG_OUT << "n_i = " << n_i->read().to_string(sc_dt::SC_HEX) << std::endl;

    u512 gn = compute_gn(h_i->read(), block_i->read(), n_i->read());

    h_nx_o.write(gn);
    n_nx_o.write(n_i->read() + u512{512});
    sigma_nx_o.write(sigma_i->read() + block_i->read());
}

void Stage::stage3()
{
    u512 block = block_i->read();
    u512 n;
    u512 h;
    u512 sigma;

    block |= u512{01} << (block_size_i->read() * 8);

    DEBUG_OUT << "h_i = " << h_i->read().to_string(sc_dt::SC_HEX) << std::endl;
    DEBUG_OUT << "n_i = " << n_i->read().to_string(sc_dt::SC_HEX) << std::endl;
    DEBUG_OUT << "sigma_i = " << sigma_i->read().to_string(sc_dt::SC_HEX) << std::endl;
    DEBUG_OUT << "block_i = " << block_i->read().to_string(sc_dt::SC_HEX) << std::endl;
    DEBUG_OUT << "Padded block: " << block.to_string(sc_dt::SC_HEX) << std::endl;

    h = compute_gn(h_i->read(), block, n_i->read());
    n = n_i->read() + u512{block_size_i->read() * 8};
    sigma = sigma_i->read() + block;
    
    h = compute_gn(h, n, 0);
    h = compute_gn(h, sigma, 0);

    h_nx_o.write(h);
    n_nx_o.write(n);
    sigma_nx_o.write(sigma);
}

void Stage::advance_state(State next_state)
{
    state_o.write(next_state);
}

};

