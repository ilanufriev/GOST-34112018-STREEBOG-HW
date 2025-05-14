#include <control_logic.hxx>
#include <utils.hxx>
#include <common.hxx>
#include <systemc>
#include <iostream>

namespace streebog_hw
{

ControlLogic::ControlLogic(sc_core::sc_module_name const &name)
    : AUTONAME(start_i)
    , AUTONAME(reset_i)
    , AUTONAME(block_i)
    , AUTONAME(block_size_i)
    , AUTONAME(hash_size_i)
    , AUTONAME(ack_i)
    , AUTONAME(clk_i)
    , AUTONAME(state_o)
    , AUTONAME(hash_o)
    , AUTONAME(sigma_nx_i)
    , AUTONAME(n_nx_i)
    , AUTONAME(h_nx_i)
    , AUTONAME(st_state_i)
    , AUTONAME(st_block_o)
    , AUTONAME(st_block_size_o)
    , AUTONAME(sigma_o)
    , AUTONAME(n_o)
    , AUTONAME(h_o)
    , AUTONAME(st_ack_o)
    , AUTONAME(st_start_o)
{
    state_o.bind(state_s_);
    hash_o.bind(hash_s_);
    st_block_o.bind(st_block_s_);
    st_block_size_o.bind(st_block_size_s_);
    sigma_o.bind(sigma_s_);
    n_o.bind(n_s_);
    h_o.bind(h_s_);
    st_ack_o.bind(st_ack_s_);
    st_start_o.bind(st_start_s_);

    SC_THREAD(thread);

    state_s_.write(ControlLogic::State::CLEAR);
}

void ControlLogic::trace(sc_core::sc_trace_file *tf)
{
    sc_core::sc_trace(tf, state_s_, "ControlLogic.state");
    sc_core::sc_trace(tf, hash_s_, "ControlLogic.hash");
    sc_core::sc_trace(tf, st_block_s_, "ControlLogic.st_block");
    sc_core::sc_trace(tf, st_block_size_s_, "ControlLogic.st_block_size");
    sc_core::sc_trace(tf, sigma_s_, "ControlLogic.sigma");
    sc_core::sc_trace(tf, n_s_, "ControlLogic.n");
    sc_core::sc_trace(tf, h_s_, "ControlLogic.h");
    sc_core::sc_trace(tf, st_ack_s_, "ControlLogic.st_ack");
    sc_core::sc_trace(tf, st_start_s_, "ControlLogic.st_start");
    sc_core::sc_trace(tf, st_sel_s_, "ControlLogic.st_sel");
}

void ControlLogic::thread()
{
    while (true)
    {
        switch (static_cast<ControlLogic::State>(state_s_.read().to_int()))
        {
            case State::CLEAR:
                {
                    hash_s_.write(0);
                    st_block_s_.write(0);
                    st_block_size_s_.write(0);
                    sigma_s_.write(0);
                    n_s_.write(0);
                    h_s_.write(0);
                    st_ack_s_.write(0);
                    st_start_s_.write(0);
                    st_sel_s_.write(0);

                    DEBUG_OUT << "State CLEAR" << std::endl;
                    WAIT_WHILE_CLK(start_i->read() == 0, 
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

                    sigma_s_.write(sigma_);
                    n_s_.write(n_);
                    h_s_.write(h_);

                    st_block_s_.write(block_);
                    st_block_size_s_.write(block_size_);

                    st_start_s_.write(1);

                    sc_core::wait(clk_i->posedge_event());

                    st_start_s_.write(0);

                    WAIT_WHILE_CLK(st_state_i->read() != Stage::State::DONE,
                                   clk_i->posedge_event());

                    sigma_ = sigma_nx_i->read();
                    n_     = n_nx_i->read();
                    h_     = h_nx_i->read();

                    DEBUG_OUT << "sigma_nx_i = " << sigma_nx_i->read().to_string(sc_dt::SC_HEX) << std::endl;
                    DEBUG_OUT << "n_nx_i = " << n_nx_i->read().to_string(sc_dt::SC_HEX) << std::endl;
                    DEBUG_OUT << "h_nx_i = " << h_nx_i->read().to_string(sc_dt::SC_HEX) << std::endl;

                    st_ack_s_.write(1);

                    sc_core::wait(clk_i->posedge_event());

                    st_ack_s_.write(0);

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
                    WAIT_WHILE_CLK(start_i->read() == 0,
                                   clk_i->posedge_event());

                    advance_state(State::BUSY);
                    break;
                }
            case State::DONE:
                {
                    DEBUG_OUT << "State DONE" << std::endl;
                    WAIT_WHILE_CLK(ack_i->read() == 0,
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
    state_s_.write(next_state);
}

};
