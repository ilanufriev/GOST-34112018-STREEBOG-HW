#include <control_logic.hxx>
#include <utils.hxx>
#include <common.hxx>
#include <systemc>
#include <iostream>

namespace streebog_hw
{

ControlLogic::ControlLogic(sc_core::sc_module_name const &name)
{
    SC_THREAD(thread);
}

void ControlLogic::trace(sc_core::sc_trace_file *tf)
{
    sc_core::sc_trace(tf, state_o, state_o.name());
    sc_core::sc_trace(tf, hash_o, hash_o.name());
    sc_core::sc_trace(tf, st_block_o, st_block_o.name());
    sc_core::sc_trace(tf, st_block_size_o, st_block_size_o.name());
    sc_core::sc_trace(tf, sigma_o, sigma_o.name());
    sc_core::sc_trace(tf, n_o, n_o.name());
    sc_core::sc_trace(tf, h_o, h_o.name());
    sc_core::sc_trace(tf, st_trg_o, st_trg_o.name());
}

void ControlLogic::thread()
{
    while (true)
    {
        switch (static_cast<ControlLogic::State>(state_o->read().to_int()))
        {
            case State::CLEAR:
                {
                    hash_o.write(0);
                    st_block_o.write(0);
                    st_block_size_o.write(0);
                    sigma_o.write(0);
                    n_o.write(0);
                    h_o.write(0);
                    st_trg_o.write(0);

                    DEBUG_OUT << "State CLEAR" << std::endl;
                    WAIT_WHILE_CLK(trg_i->read() == 0, 
                                   clk_i->posedge_event());

                    hash_size_ = hash_size_i->read();

                    sigma_ = 0;
                    n_     = 0;
                    h_     = hash_size_ == 0 ? INIT_VECTOR_512
                                             : INIT_VECTOR_256;

                    advance_state(State::BUSY);
                    break;
                }
            case State::BUSY:
                {
                    State next_state;

                    DEBUG_OUT << "State BUSY" << std::endl;
                    block_      = block_i->read();
                    block_size_ = block_size_i->read();

                    if (block_size_ == 64)
                    {
                        next_state = State::READY;
                    }
                    else
                    {
                        next_state = State::DONE;
                    }

                    sigma_o.write(sigma_);
                    n_o.write(n_);
                    h_o.write(h_);

                    st_block_o.write(block_);
                    st_block_size_o.write(block_size_);

                    st_trg_o.write(1);

                    sc_core::wait(clk_i->posedge_event());

                    st_trg_o.write(0);

                    WAIT_WHILE_CLK(st_state_i->read() != Stage::State::DONE,
                                   clk_i->posedge_event());

                    sigma_ = sigma_nx_i->read();
                    n_     = n_nx_i->read();
                    h_     = h_nx_i->read();

                    DEBUG_OUT << "sigma_nx_i = " << sigma_nx_i->read().to_string(sc_dt::SC_HEX) << std::endl;
                    DEBUG_OUT << "n_nx_i = " << n_nx_i->read().to_string(sc_dt::SC_HEX) << std::endl;
                    DEBUG_OUT << "h_nx_i = " << h_nx_i->read().to_string(sc_dt::SC_HEX) << std::endl;

                    st_trg_o.write(1);

                    sc_core::wait(clk_i->posedge_event());

                    st_trg_o.write(0); // This will reset Stage to CLEAR

                    if (next_state == State::DONE)
                    {
                        hash_o->write(h_);
                    }

                    advance_state(next_state);
                    break;
                }
            case State::READY:
                {
                    DEBUG_OUT << "State READY" << std::endl;
                    WAIT_WHILE_CLK(trg_i->read() == 0,
                                   clk_i->posedge_event());

                    advance_state(State::BUSY);
                    break;
                }
            case State::DONE:
                {
                    DEBUG_OUT << "State DONE" << std::endl;
                    WAIT_WHILE_CLK(trg_i->read() == 0,
                                   clk_i->posedge_event());

                    advance_state(State::CLEAR);
                    break;
                }
        }

        sc_core::wait(clk_i->posedge_event());
    }
}

void ControlLogic::advance_state(State next_state)
{
    state_o.write(next_state);
}

};
