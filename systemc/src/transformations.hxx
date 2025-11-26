#ifndef __G_N__
#define __G_N__

#include <common.hxx>
#include <datatypes.hxx>
#include <sysc/kernel/sc_module.h>
#include <sysc/kernel/sc_module_name.h>
#include <systemc>

namespace streebog_hw
{

struct PTransform : public sc_core::sc_module
{
    PTransform(sc_core::sc_module_name const &name);

    void method();

    void trace(sc_core::sc_trace_file *tf);

    sc_core::sc_in<u512> a_i       {"a_i"};
    sc_core::sc_out<u512> result_o {"result_o"};
};

struct SLTransform : public sc_core::sc_module
{
    SLTransform(sc_core::sc_module_name const &name);

    void method();

    void trace(sc_core::sc_trace_file *tf);

    sc_core::sc_in<u512> a_i       {"a_i"};
    sc_core::sc_out<u512> result_o {"result_o"};
};

struct Gn : public sc_core::sc_module
{
    using ScState = sc_dt::sc_uint<32>;

    enum State
    {
        CLEAR,
        BUSY,
        DONE
    };

    Gn(sc_core::sc_module_name const &name);
 
    void thread();

    void trace(sc_core::sc_trace_file *tf);

    sc_core::sc_in<u512> m_i   {"m_i"};
    sc_core::sc_in<u512> n_i   {"n_i"};
    sc_core::sc_in<u512> h_i   {"h_i"};
    sc_core::sc_in<bool> trg_i {"trg_i"};
    sc_core::sc_in<bool> clk_i {"clk_i"};

    sc_core::sc_out<u512> result_o   {"result_o"};
    sc_core::sc_out<ScState> state_o {"state_o"};

    sc_core::sc_in<u512> sl_tr_result_i {"sl_tr_result_i"};
    sc_core::sc_in<u512> p_tr_result_i  {"p_tr_result_i"};

    sc_core::sc_out<u512> sl_tr_a_o {"sl_tr_a_o"};
    sc_core::sc_out<u512> p_tr_a_o  {"p_tr_a_o"};

    const std::vector<EventTableEntry> &get_events() const;
private:
    void advance_state(State next_state);
    u512 compute_gn();

    std::vector<EventTableEntry> events_;
};

extern const uint64_t sl_precomp_table[8][256];

}

#endif // __G_N__
