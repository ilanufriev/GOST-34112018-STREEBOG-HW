#ifndef __STAGE_HXX__
#define __STAGE_HXX__

#include <datatypes.hxx>
#include <transformations.hxx>
#include <common.hxx>
#include <systemc>

namespace streebog_hw
{

struct Stage : public sc_core::sc_module
{
    using ScState = sc_dt::sc_uint<32>;

    enum State {
        CLEAR,
        BUSY,
        DONE
    };

    Stage(sc_core::sc_module_name const &name);

    void thread();
    
    in_port<bool> start_i;
    in_port<u512> block_i;
    in_port<u8>   block_size_i;
    in_port<u512> sigma_i;
    in_port<u512> n_i;
    in_port<u512> h_i;
    in_port<bool> ack_i;
    in_port<bool> clk_i;
    
    out_export<u512>  sigma_nx_o;
    out_export<u512>  n_nx_o;
    out_export<u512>  h_nx_o;
    out_export<ScState> state_o;

    in_port<u512> g_n_result_i;
    in_port<Gn::ScState> g_n_state_i;
    out_export<u512> g_n_m_o;
    out_export<u512> g_n_n_o;
    out_export<u512> g_n_h_o;
    out_export<bool> g_n_start_o;
    out_export<bool> g_n_ack_o;

    void trace(sc_core::sc_trace_file *tf);
private:
    void stage2();
    void stage3();
    u512 compute_gn(const u512 &h, const u512 &m, const u512 &n);
    void advance_state(State next_state);

    u512 h_ = 0;
    u512 n_ = 0;
    u512 sigma_ = 0;

    sc_core::sc_signal<u512>  sigma_nx_s_;
    sc_core::sc_signal<u512>  n_nx_s_;
    sc_core::sc_signal<u512>  h_nx_s_;
    sc_core::sc_signal<ScState> state_s_;

    sc_core::sc_signal<u512> g_n_m_s_;
    sc_core::sc_signal<u512> g_n_n_s_;
    sc_core::sc_signal<u512> g_n_h_s_;
    sc_core::sc_signal<bool> g_n_start_s_;
    sc_core::sc_signal<bool> g_n_ack_s_;
};

}

#endif // __STAGE_HXX__
