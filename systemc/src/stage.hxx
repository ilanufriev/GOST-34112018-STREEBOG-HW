#ifndef __STAGE_HXX__
#define __STAGE_HXX__

#include <datatypes.hxx>
#include <common.hxx>
#include <systemc>

namespace streebog_hw
{

struct Stage : public sc_core::sc_module
{
    enum State {
        CLEAR,
        BUSY,
        DONE
    };

    Stage(sc_core::sc_module_name const &name);

    void thread();
    void stage2();
    void stage3();
    
    in_port<u512> block_i;
    in_port<u8>   block_size_i;
    in_port<u512> sigma_i;
    in_port<u512> n_i;
    in_port<u512> h_i;
    in_port<bool> ack_i;
    in_port<bool> start_i;
    in_port<bool> sel_i;
    
    out_export<u512>  sigma_nx_o;
    out_export<u512>  n_nx_o;
    out_export<u512>  h_nx_o;
    out_export<State> state_o;

private:

    void advance_state(State next_state);

    sc_core::sc_signal<u512>  sigma_nx_s_;
    sc_core::sc_signal<u512>  n_nx_s_;
    sc_core::sc_signal<u512>  h_nx_s_;
    sc_core::sc_signal<State> state_s_;
};

}

#endif // __STAGE_HXX__
