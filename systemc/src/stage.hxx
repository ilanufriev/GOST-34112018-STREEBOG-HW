#ifndef __STAGE_HXX__
#define __STAGE_HXX__

#include <datatypes.hxx>
#include <transformations.hxx>
#include <common.hxx>
#include <systemc>
#include <vector>

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

    sc_core::sc_in<bool> trg_i        {"trg_i"};
    sc_core::sc_in<u512> block_i      {"block_i"};
    sc_core::sc_in<u8>   block_size_i {"block_size_i"};
    sc_core::sc_in<u512> sigma_i      {"sigma_i"};
    sc_core::sc_in<u512> n_i          {"n_i"};
    sc_core::sc_in<u512> h_i          {"h_i"};
    sc_core::sc_in<bool> clk_i        {"clk_i"};

    sc_core::sc_out<u512>  sigma_nx_o {"sigma_nx_o"};
    sc_core::sc_out<u512>  n_nx_o     {"n_nx_o"};
    sc_core::sc_out<u512>  h_nx_o     {"h_nx_o"};
    sc_core::sc_out<ScState> state_o  {"state_o"};

    sc_core::sc_in<u512> g_n_result_i       {"g_n_result_i"};
    sc_core::sc_in<Gn::ScState> g_n_state_i {"g_n_state_i"};
    sc_core::sc_out<u512> g_n_m_o           {"g_n_m_o"};
    sc_core::sc_out<u512> g_n_n_o           {"g_n_n_o"};
    sc_core::sc_out<u512> g_n_h_o           {"g_n_h_o"};
    sc_core::sc_out<bool> g_n_trg_o         {"g_n_trg_o"};

    void trace(sc_core::sc_trace_file *tf);
    const std::vector<EventTableEntry> &get_events() const;
private:
    void stage2();
    void stage3();
    u512 compute_gn(const u512 &h, const u512 &m, const u512 &n);
    void advance_state(State next_state);

    u512 h_ = 0;
    u512 n_ = 0;
    u512 sigma_ = 0;

    std::vector<EventTableEntry> events_;
};

}

#endif // __STAGE_HXX__
