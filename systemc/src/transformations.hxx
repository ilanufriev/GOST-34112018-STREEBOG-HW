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

    in_port<u512> a_i;

    out_export<u512> result_o;
private:
    sc_core::sc_signal<u512> result_s_;
};

struct SLTransform : public sc_core::sc_module
{
    SLTransform(sc_core::sc_module_name const &name);

    void method();

    void trace(sc_core::sc_trace_file *tf);

    in_port<u512> a_i;

    out_export<u512> result_o;
private:
    sc_core::sc_signal<u512> result_s_;
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

    in_port<u512> m_i;
    in_port<u512> n_i;
    in_port<u512> h_i;
    in_port<bool> trg_i;
    in_port<bool> clk_i;

    out_export<u512> result_o;
    out_export<ScState> state_o;

    in_port<u512> sl_tr_result_i;
    in_port<u512> p_tr_result_i;

    out_export<u512> sl_tr_a_o;
    out_export<u512> p_tr_a_o;

private:
    void advance_state(State next_state);
    u512 compute_gn();

    sc_core::sc_signal<u512> result_s_;
    sc_core::sc_signal<ScState> state_s_;

    sc_core::sc_signal<u512> sl_tr_a_s_;
    sc_core::sc_signal<u512> p_tr_a_s_;
};

extern const uint64_t sl_precomp_table[8][256];

}

#endif // __G_N__
