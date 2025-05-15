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
    sc_core::sc_in<bool> trg_i        {"trg_i"};
    sc_core::sc_in<bool> reset_i      {"reset_i"};
    sc_core::sc_in<u512> block_i      {"block_i"};
    sc_core::sc_in<u8>   block_size_i {"block_size_i"};
    sc_core::sc_in<bool> hash_size_i  {"hash_size_i"};
    sc_core::sc_in<bool> clk_i        {"clk_i"};

    sc_core::sc_out<ScState> state_o {"state_o"};
    sc_core::sc_out<u512>    hash_o  {"hash_o"};

    // Stage block side of I/O
    sc_core::sc_in<u512>           sigma_nx_i {"sigma_nx_i"};
    sc_core::sc_in<u512>           n_nx_i     {"n_nx_i"};
    sc_core::sc_in<u512>           h_nx_i     {"h_nx_i"};
    sc_core::sc_in<Stage::ScState> st_state_i {"st_state_i"};

    sc_core::sc_out<u512> st_block_o      {"st_block_o"};
    sc_core::sc_out<u8>   st_block_size_o {"st_block_size_o"};
    sc_core::sc_out<u512> sigma_o         {"sigma_o"};
    sc_core::sc_out<u512> n_o             {"n_o"};
    sc_core::sc_out<u512> h_o             {"h_o"};
    sc_core::sc_out<bool> st_trg_o        {"st_trg_o"};

    void trace(sc_core::sc_trace_file *tf);

private:
    void advance_state(State next_state);

    u512 block_;
    u8   block_size_;
    bool hash_size_;

    u512 sigma_;
    u512 h_;
    u512 n_;
};

}

#endif // __CONTROL_LOGIC_HXX__

