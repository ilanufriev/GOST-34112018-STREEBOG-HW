#ifndef __CONTROL_LOGIC_HXX__
#define __CONTROL_LOGIC_HXX__

#include <datatypes.hxx>
#include <systemc>
#include <stage.hxx>

namespace streebog_hw
{

struct ControlLogic : public sc_core::sc_module
{
    using ScState = sc_dt::sc_uint<32>;
        
    enum State
    {
        CLEAR,
        BUSY,
        READY,
        DONE
    };

    static constexpr unsigned long long STAGES_COUNT = 2;
    static constexpr unsigned long long STAGE2_IDX   = 0;
    static constexpr unsigned long long STAGE3_IDX   = 1;

    ControlLogic(sc_core::sc_module_name const &name);
    
    void thread();

    // Initiator side of I/O
    in_port<bool> start_i;
    in_port<bool> reset_i;
    in_port<u512> block_i;
    in_port<u8>   block_size_i;
    in_port<bool> hash_size_i;
    in_port<bool> ack_i;
    in_port<bool> clk_i;

    out_export<ScState> state_o;
    out_export<u512>  hash_o;

    // Stage block side of I/O
    in_port<u512>         sigma_nx_i;
    in_port<u512>         n_nx_i;
    in_port<u512>         h_nx_i;
    in_port<Stage::ScState> st_state_i;

    out_export<u512> st_block_o;
    out_export<u8>   st_block_size_o;
    out_export<u512> sigma_o;
    out_export<u512> n_o;
    out_export<u512> h_o;
    out_export<bool> st_ack_o;
    out_export<bool> st_start_o;

    void trace(sc_core::sc_trace_file *tf);

private:
    void advance_state(State next_state);

    u512 block_;
    u8   block_size_;
    bool hash_size_;

    u512 sigma_;
    u512 h_;
    u512 n_;

    sc_core::sc_signal<ScState> state_s_;
    sc_core::sc_signal<u512>  hash_s_;

    sc_core::sc_signal<u512>  st_block_s_;
    sc_core::sc_signal<u8>    st_block_size_s_;
    sc_core::sc_signal<u512>  sigma_s_;
    sc_core::sc_signal<u512>  n_s_;
    sc_core::sc_signal<u512>  h_s_;
    sc_core::sc_signal<bool>  st_ack_s_;
    sc_core::sc_signal<bool>  st_start_s_;
    sc_core::sc_signal<bool>  st_sel_s_;
};

}

#endif // __CONTROL_LOGIC_HXX__

