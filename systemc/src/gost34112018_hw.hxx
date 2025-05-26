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
    using ScState = sc_dt::sc_uint<32>;
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

    sc_core::sc_in<bool> clk_i                     {"clk_i"};
    sc_core::sc_in<bool> trg_i                     {"trg_i"};
    sc_core::sc_in<bool> reset_i                   {"reset_i"};
    sc_core::sc_in<sc_dt::sc_biguint<512>> block_i {"block_i"};
    sc_core::sc_in<sc_dt::sc_uint<8>> block_size_i {"block_size_i"};
    sc_core::sc_in<bool> hash_size_i               {"hash_size_i"};

    sc_core::sc_out<ScState> state_o               {"state_o"};
    sc_core::sc_out<sc_dt::sc_biguint<512>> hash_o {"hash_o"};

    std::vector<EventTableEntry> get_events() const;
private:
    using u512 = sc_dt::sc_biguint<512>;
    using u8   = sc_dt::sc_uint<8>;

    void advance_state(State next_state);

    // Submodules
    std::unique_ptr<ControlLogic> cl_;
    std::unique_ptr<Stage>        st_;
    std::unique_ptr<Gn>           gn_;
    std::unique_ptr<SLTransform>  sl_;
    std::unique_ptr<PTransform>   p_;

    // Connector signals
    // ControlLogic to Stage I/O
    sc_core::sc_signal<u512> cl_st_sigma_nx_;
    sc_core::sc_signal<u512> cl_st_n_nx_;
    sc_core::sc_signal<u512> cl_st_h_nx_;
    sc_core::sc_signal<Stage::ScState> cl_st_state_;

    sc_core::sc_signal<u512> cl_st_block_;
    sc_core::sc_signal<u8>   cl_st_block_size_;
    sc_core::sc_signal<u512> cl_st_sigma_;
    sc_core::sc_signal<u512> cl_st_n_;
    sc_core::sc_signal<u512> cl_st_h_;
    sc_core::sc_signal<bool> cl_st_trg_;

    // Stage to Gn I/O
    sc_core::sc_signal<u512> st_g_n_result_;
    sc_core::sc_signal<Gn::ScState> st_g_n_state_;
    sc_core::sc_signal<u512> st_g_n_m_;
    sc_core::sc_signal<u512> st_g_n_n_;
    sc_core::sc_signal<u512> st_g_n_h_;
    sc_core::sc_signal<bool> st_g_n_trg_;

    // Gn to P and SL I/O
    sc_core::sc_signal<u512> g_n_sl_tr_result_;
    sc_core::sc_signal<u512> g_n_p_tr_result_;

    sc_core::sc_signal<u512> g_n_sl_tr_a_;
    sc_core::sc_signal<u512> g_n_p_tr_a_;
};

}


#endif // __GOST34112018_HW_HXX__
