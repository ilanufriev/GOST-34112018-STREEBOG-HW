#ifndef __GOST34112018_HW_HXX__
#define __GOST34112018_HW_HXX__

#include <control_logic.hxx>
#include <memory>
#include <stage.hxx>
#include <sysc/communication/sc_export.h>
#include <sysc/communication/sc_signal.h>
#include <sysc/communication/sc_signal_ifs.h>
#include <transformations.hxx>
#include <sysc/datatypes/int/sc_biguint.h>
#include <systemc>

namespace streebog_hw
{

struct Gost34112018_Hw : public sc_core::sc_module
{
    enum State
    {
        CLEAR,
        BUSY,
        READY,
        DONE
    };
    
    Gost34112018_Hw(sc_core::sc_module_name const &name);

    void thread();
    void trace(sc_core::sc_trace_file *tf);
    u512 read_hash() const;
    State read_state() const;
    
    sc_core::sc_signal<bool> clk_i;
    sc_core::sc_signal<bool> start_i;
    sc_core::sc_signal<bool> reset_i;
    sc_core::sc_signal<bool> ack_i;
    sc_core::sc_signal<sc_dt::sc_biguint<512>> block_i;
    sc_core::sc_signal<sc_dt::sc_uint<8>>      block_size_i;
    sc_core::sc_signal<bool>                   hash_size_i;

private:
    void advance_state(State next_state);

    const sc_core::sc_export<sc_core::sc_signal_out_if<sc_dt::sc_biguint<512>>> *hash_o;
    const sc_core::sc_export<sc_core::sc_signal_out_if<ControlLogic::ScState>>    *state_o;

    // Submodules
    std::unique_ptr<ControlLogic> cl_;
    std::unique_ptr<Stage>        st_;
    std::unique_ptr<Gn>           gn_;
    std::unique_ptr<SLTransform>  sl_;
    std::unique_ptr<PTransform>   p_;
};

}


#endif // __GOST34112018_HW_HXX__
