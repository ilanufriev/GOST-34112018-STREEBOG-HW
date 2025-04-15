#include <utils.hxx>
#include <systemc>
#include <tlm_core/tlm_2/tlm_generic_payload/tlm_gp.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <iostream>

namespace streebog_hw
{

struct ControlLogic : public sc_core::sc_module
{
    ControlLogic(sc_core::sc_module_name const& name)
        : socket("socket")
    {
        socket.register_nb_transport_bw(this, &ControlLogic::nb_transport_bw);
    }
    
    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload &payload, 
                                       tlm::tlm_phase           &phase, 
                                       sc_core::sc_time         &delta)
    {
        METHOD_NOT_IMPLEMENTED;
        return tlm::tlm_sync_enum::TLM_COMPLETED;
    }

    void control_logic_thread()
    {
        tlm::tlm_generic_payload payload;
        payload.set_address(0);
        payload.set_command(tlm::tlm_command::TLM_READ_COMMAND);
        payload.set_data_ptr(unsigned char *data)
    }

    tlm_utils::simple_initiator_socket<ControlLogic> socket;
};

};
