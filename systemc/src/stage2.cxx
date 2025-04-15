#include <systemc>
#include <tlm_core/tlm_2/tlm_generic_payload/tlm_gp.h>
#include <tlm_core/tlm_2/tlm_generic_payload/tlm_phase.h>
#include <tlm_utils/simple_target_socket.h>
#include <iostream>
#include <utils.hxx>

namespace streebog_hw
{

struct Stage2 : public sc_core::sc_module
{
    Stage2(sc_core::sc_module_name const& name)
        : socket("socket")
    {
        socket.register_b_transport(this, &Stage2::b_transport);
        socket.register_nb_transport_fw(this, &Stage2::nb_transport_fw);
    }
    
    void b_transport(tlm::tlm_generic_payload &payload,
                     sc_core::sc_time         &delta)
    {

    }

    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload &payload,
                                       tlm::tlm_phase           &phase,
                                       sc_core::sc_time         &delta)
    {
        METHOD_NOT_IMPLEMENTED;
        return tlm::tlm_sync_enum::TLM_COMPLETED;
    }

    tlm_utils::simple_target_socket<Stage2> socket;
};

};

