#include "datatypes.hxx"
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
    sc_core::sc_trace(tf, clk_i, clk_i.name());
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

                    DEBUG_OUT << "State CLEAR" << " at " << g_clock_counter << " clks" << std::endl;
                    WAIT_WHILE_CLK_EXPR(trg_i->read() == 0, 
                                    clk_i->posedge_event(),
                                    events_.emplace_back(g_clock_counter, "Waiting for trg", this->name()));

                    hash_size_ = hash_size_i->read();

                    sigma_ = 0;
                    n_     = 0;
                    h_     = hash_size_ == 0 ? INIT_VECTOR_512
                                             : INIT_VECTOR_256;

                    advance_state(State::BUSY);
                    break; // Do not wait for a next posedge.
                }
            case State::BUSY:
                {
                    State next_state;

                    DEBUG_OUT << "State BUSY" << " at " << g_clock_counter << " clks" << std::endl;
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
                    
                    events_.emplace_back(g_clock_counter, "Triggering stage", this->name());
                    sc_core::wait(clk_i->posedge_event());

                    st_trg_o.write(0);

                    WAIT_WHILE_CLK_EXPR(st_state_i->read() != Stage::State::BUSY,
                        clk_i->posedge_event(),
                            events_.emplace_back(g_clock_counter, 
                                                 "Waiting for the stage to become busy", this->name()));

                    WAIT_WHILE_CLK_EXPR(st_state_i->read() != Stage::State::DONE,
                        clk_i->posedge_event(),
                        events_.emplace_back(g_clock_counter,
                                                "Waiting for the stage to become done", this->name()));

                    sigma_ = sigma_nx_i->read();
                    n_     = n_nx_i->read();
                    h_     = h_nx_i->read();

                    if (next_state == State::DONE)
                    {
                        hash_o->write(h_);
                    }

                    advance_state(next_state);
                    break;
                }
            case State::READY:
                {
                    DEBUG_OUT << "State READY" << " at " << g_clock_counter << " clks" << std::endl;
                    WAIT_WHILE_CLK_EXPR(trg_i->read() == 0,
                        clk_i->posedge_event(),
                            events_.emplace_back(g_clock_counter,
                                                 "Waiting for trg in READY", this->name()));

                    advance_state(State::BUSY);
                    break;
                }
            case State::DONE:
                {
                    DEBUG_OUT << "State DONE" << " at " << g_clock_counter << " clks" << std::endl;
                    WAIT_WHILE_CLK_EXPR(trg_i->read() == 0,
                        clk_i->posedge_event(),
                        events_.emplace_back(g_clock_counter,
                                             "Waiting for trg in DONE", this->name()));

                    advance_state(State::CLEAR);
                    break;
                }
        }

        events_.emplace_back(g_clock_counter, "Transitioning to another state", this->name());
        sc_core::wait(clk_i->posedge_event());
    }
}

const std::vector<EventTableEntry> &ControlLogic::get_events() const
{
    return events_;
}

void ControlLogic::advance_state(State next_state)
{
    state_o.write(next_state);
}

};
