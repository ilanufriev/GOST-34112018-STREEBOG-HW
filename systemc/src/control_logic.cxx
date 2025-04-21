#include <control_logic.hxx>
#include <payloads.hxx>
#include <utils.hxx>
#include <common.hxx>
#include <string>
#include <systemc>
#include <iostream>
#include <tlm_core/tlm_2/tlm_generic_payload/tlm_gp.h>
#include <tlm_utils/simple_initiator_socket.h>

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
    h_o.bind(h_o);
    st_ack_o.bind(st_ack_s_);
    st_start_o.bind(st_start_s_);
    st_sel_o.bind(st_sel_s_);

    state_s_.write(ControlLogic::State::CLEAR);

    SC_THREAD(thread);
}

void ControlLogic::thread()
{
    State next_state;
    while (true)
    {
        switch (state_s_.read())
        {
            case State::CLEAR:
                {
                    DEBUG_OUT << "State CLEAR" << std::endl;
                    WAIT_WHILE(start_i->read() == 0);
                    
                    sigma_ = 0;
                    n_     = 0;
                    h_     = hash_size_ == 0 ? INIT_VECTOR_512 
                                             : INIT_VECTOR_256;
                    state_s_ = State::BUSY;

                    break;
                }
            case State::BUSY:
                {
                    DEBUG_OUT << "State BUSY" << std::endl;
                    block_      = block_i->read();
                    block_size_ = block_i->read();
                    hash_size_  = hash_size_i->read();

                    if (block_size_ == 64)
                    {
                        st_sel_s_.write(0);
                        next_state = State::READY;
                    }
                    else 
                    {
                        st_sel_s_.write(1);
                        next_state = State::BUSY;
                    }

                    sigma_s_.write(sigma_);
                    n_s_.write(n_);
                    h_s_.write(h_);

                    st_block_s_.write(block_);
                    st_block_size_s_.write(block_size_);
                    
                    st_start_s_.write(1);

                    WAIT_WHILE(st_state_i->read() != State::BUSY);

                    st_start_s_.write(0);

                    WAIT_WHILE(st_state_i->read() != State::DONE);

                    sigma_ = sigma_nx_i->read();
                    n_     = n_nx_i->read();
                    h_     = h_nx_i->read();

                    st_ack_s_.write(1);
                    
                    WAIT_WHILE(st_state_i->read() != State::CLEAR);

                    st_ack_s_.write(0);
                    
                    state_s_.write(next_state);
                    break;
                }
            case State::READY:
                {
                    DEBUG_OUT << "State READY" << std::endl;
                    WAIT_WHILE(start_i->read() == 0);

                    state_s_.write(State::BUSY);
                    break;
                }
            case State::DONE:
                {
                    DEBUG_OUT << "State DONE" << std::endl;
                    WAIT_WHILE(ack_i->read() == 0);

                    state_s_.write(State::CLEAR);
                    break;
                }
        }
    }
}

};
