#ifndef __STAGE_HXX__
#define __STAGE_HXX__

#include <datatypes.hxx>
#include <common.hxx>
#include <systemc>
#include <gost34112018.h>

namespace streebog_hw
{

struct Stage : public sc_core::sc_module
{
    Stage(sc_core::sc_module_name const &name);

    void thread();

    in_port<gost_u512> st_block_i;
    in_port<gost_u8>   st_block_size_i;
    in_port<gost_u512> sigma_i;
    in_port<gost_u512> n_i;
    in_port<gost_u512> h_i;
    in_port<bool>      st_ack_i;
    in_port<bool>      st_start_i;
    in_port<bool>      st_sel_i;
    
    out_export<
private:
};

}

#endif // __STAGE_HXX__
