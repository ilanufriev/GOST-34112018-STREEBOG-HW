#ifndef __CONTROL_LOGIC_HXX__
#define __CONTROL_LOGIC_HXX__

#include "datatypes.hxx"
#include <systemc>

namespace streebog_hw
{

struct ControlLogic : public sc_core::sc_module
{
    
    enum class State
    {
        CLEAR,
        BUSY,
        READY,
        DONE
    };

    static constexpr unsigned long long STAGES_COUNT = 2;
    static constexpr unsigned long long STAGE2_IDX   = 0;
    static constexpr unsigned long long STAGE3_IDX   = 1;

    ControlLogic(sc_core::sc_module_name const& name);
    
    void thread();

    // Initiator side of I/O
    in_port<bool>         start_i;
    in_port<bool>         reset_i;
    in_port<gost_u512>    block_i;
    in_port<gost_u8>      block_size_i;
    in_port<bool>         hash_size_i;
    in_port<bool>         ack_i;
    
    out_export<State>     state_o;
    out_export<gost_u512> hash_o;

    // Stage blocks side of I/O
    in_port<gost_u512>    sigma_nx_i;
    in_port<gost_u512>    n_nx_i;
    in_port<gost_u512>    h_nx_i;
    in_port<State>        st_state_i;

    out_export<gost_u512> st_block_o;
    out_export<gost_u8>   st_block_size_o;
    out_export<gost_u512> sigma_o;
    out_export<gost_u512> n_o;
    out_export<gost_u512> h_o;
    out_export<bool>      st_ack_o;
    out_export<bool>      st_start_o;
    out_export<bool>      st_sel_o;

    ~ControlLogic();

private:
    gost_u512 block_;
    gost_u8   block_size_;
    bool      hash_size_;

    gost_u512 sigma_;
    gost_u512 h_;
    gost_u512 n_;

    sc_core::sc_signal<State>     state_s_;
    sc_core::sc_signal<gost_u512> hash_s_;

    sc_core::sc_signal<gost_u512> st_block_s_;
    sc_core::sc_signal<gost_u8>   st_block_size_s_;
    sc_core::sc_signal<gost_u512> sigma_s_;
    sc_core::sc_signal<gost_u512> n_s_;
    sc_core::sc_signal<gost_u512> h_s_;
    sc_core::sc_signal<bool>      st_ack_s_;
    sc_core::sc_signal<bool>      st_start_s_;
    sc_core::sc_signal<bool>      st_sel_s_;
};

}

#endif // __CONTROL_LOGIC_HXX__

