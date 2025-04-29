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

void ControlLogic::thread()
{
    while (true)
    {
        switch (state_s_.read())
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
                    WAIT_WHILE(start_i->read() == 0);

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
                        st_sel_s_.write(0);
                        next_state = State::READY;
                    }
                    else
                    {
                        st_sel_s_.write(1);
                        next_state = State::DONE;
                    }

                    sigma_s_.write(sigma_);
                    n_s_.write(n_);
                    h_s_.write(h_);

                    st_block_s_.write(block_);
                    st_block_size_s_.write(block_size_);

                    st_start_s_.write(1);

                    WAIT_WHILE(st_state_i->read() != Stage::State::BUSY);

                    st_start_s_.write(0);

                    WAIT_WHILE(st_state_i->read() != Stage::State::DONE);

                    sigma_ = sigma_nx_i->read();
                    n_     = n_nx_i->read();
                    h_     = h_nx_i->read();

                    DEBUG_OUT << "sigma_nx_i = " << sigma_nx_i->read().to_string(sc_dt::SC_HEX) << std::endl;
                    DEBUG_OUT << "n_nx_i = " << n_nx_i->read().to_string(sc_dt::SC_HEX) << std::endl;
                    DEBUG_OUT << "h_nx_i = " << h_nx_i->read().to_string(sc_dt::SC_HEX) << std::endl;

                    st_ack_s_.write(1);

                    WAIT_WHILE(st_state_i->read() != Stage::State::CLEAR);

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
                    WAIT_WHILE(start_i->read() == 0);

                    advance_state(State::BUSY);
                    break;
                }
            case State::DONE:
                {
                    DEBUG_OUT << "State DONE" << std::endl;
                    WAIT_WHILE(ack_i->read() == 0);

                    advance_state(State::CLEAR);
                    break;
                }
        }
    }
}

void ControlLogic::advance_state(State next_state)
{
    state_s_.write(next_state);
    WAIT_WHILE(state_s_.read() != next_state);
}

};
