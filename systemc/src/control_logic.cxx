#include "control_logic.hxx"
#include "payloads.hxx"
#include <common.hxx>
#include <string>
#include <utils.hxx>
#include <systemc>
#include <tlm_core/tlm_2/tlm_generic_payload/tlm_gp.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <iostream>

namespace streebog_hw
{

ControlLogic::ControlLogic(sc_core::sc_module_name const& name)
    : ctx_next_i("ctx_i")
    , block_i("block_i")
{
    for (unsigned long long i = 0; i < STAGES_COUNT; i++)
    {
        block_o[i] = new sc_core::sc_export<sc_core::sc_signal_out_if<gost_u512>>((std::string("block_o ") + std::to_string(i)).c_str());
        ctx_o  [i] = new sc_core::sc_export<sc_core::sc_signal_out_if<gost_algctx>>((std::string("ctx_o") + std::to_string(i)).c_str());

        block_o[i]->bind(block_[i]);
        ctx_o  [i]->bind(ctx_[i]);
    }

    SC_THREAD(thread);
}

void ControlLogic::thread()
{
    
}

ControlLogic::~ControlLogic()
{
    for (unsigned long long i = 0; i < STAGES_COUNT; i++)
    {
        delete block_o[i];
        delete ctx_o  [i];
    }
}

};
