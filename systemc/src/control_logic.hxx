#ifndef __CONTROL_LOGIC_HXX__
#define __CONTROL_LOGIC_HXX__

#include "datatypes.hxx"
#include <systemc>

namespace streebog_hw
{

struct ControlLogic : public sc_core::sc_module
{
    enum class Phase
    {
        Initialization,
        Computation,
        Return,
    };

    static constexpr unsigned long long STAGES_COUNT = 2;
    static constexpr unsigned long long STAGE2_IDX   = 0;
    static constexpr unsigned long long STAGE3_IDX   = 1;

    ControlLogic(sc_core::sc_module_name const& name);
    
    void thread();

    // inputs
    sc_core::sc_port<sc_core::sc_signal_in_if<gost_algctx>>    ctx_next_i;
    sc_core::sc_port<sc_core::sc_signal_in_if<gost_u512>>      block_i;
    sc_core::sc_port<sc_core::sc_signal_in_if<gost_u8>>        size_i;

    // outputs
    sc_core::sc_export<sc_core::sc_signal_out_if<gost_u512>>   *block_o[STAGES_COUNT];
    sc_core::sc_export<sc_core::sc_signal_out_if<gost_algctx>> *ctx_o  [STAGES_COUNT];
    sc_core::sc_export<sc_core::sc_signal_out_if<gost_u8>>      size_o;
    sc_core::sc_export<sc_core::sc_signal_out_if<bool>>         stage_sel_o;

    ~ControlLogic();

private:

    // internal signals
    sc_core::sc_signal<gost_u512>   block_[STAGES_COUNT];
    sc_core::sc_signal<gost_algctx> ctx_  [STAGES_COUNT];
};

}

#endif // __CONTROL_LOGIC_HXX__

