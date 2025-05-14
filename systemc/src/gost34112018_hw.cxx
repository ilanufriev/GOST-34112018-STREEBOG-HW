#include <control_logic.hxx>
#include <utils.hxx>
#include <gost34112018_hw.hxx>

namespace streebog_hw
{

Gost34112018_Hw::Gost34112018_Hw(sc_core::sc_module_name const &name)
{
    cl_ = std::make_unique<ControlLogic>("ControlLogic");
    st_ = std::make_unique<Stage>("Stage");
    gn_ = std::make_unique<Gn>("Gn");
    sl_ = std::make_unique<SLTransform>("SL");
    p_  = std::make_unique<PTransform>("P");

    cl_->trg_i.bind(trg_i);
    cl_->reset_i.bind(reset_i);
    cl_->block_i.bind(block_i);
    cl_->block_size_i.bind(block_size_i);
    cl_->hash_size_i.bind(hash_size_i);
    cl_->clk_i.bind(clk_i);

    hash_o = &cl_->hash_o;
    state_o = &cl_->state_o;

    cl_->sigma_nx_i.bind(st_->sigma_nx_o);
    cl_->n_nx_i.bind(st_->n_nx_o);
    cl_->h_nx_i.bind(st_->h_nx_o);
    cl_->st_state_i.bind(st_->state_o);

    st_->clk_i.bind(clk_i);
    st_->trg_i.bind(cl_->st_trg_o);
    st_->block_i.bind(cl_->st_block_o);
    st_->block_size_i.bind(cl_->st_block_size_o);
    st_->sigma_i.bind(cl_->sigma_o);
    st_->n_i.bind(cl_->n_o);
    st_->h_i.bind(cl_->h_o);

    st_->g_n_result_i.bind(gn_->result_o);
    st_->g_n_state_i.bind(gn_->state_o);
    
    gn_->m_i.bind(st_->g_n_m_o);
    gn_->n_i.bind(st_->g_n_n_o);
    gn_->h_i.bind(st_->g_n_h_o);
    gn_->trg_i.bind(st_->g_n_trg_o);
    gn_->clk_i.bind(clk_i);

    gn_->sl_tr_result_i.bind(sl_->result_o);
    gn_->p_tr_result_i.bind(p_->result_o);

    sl_->a_i.bind(gn_->sl_tr_a_o);
    p_->a_i.bind(gn_->p_tr_a_o);

    SC_THREAD(thread);
}

void Gost34112018_Hw::thread()
{
}

void Gost34112018_Hw::trace(sc_core::sc_trace_file *tf)
{
    sc_core::sc_trace(tf, clk_i, "Gost34112018_Hw.clk");
    sc_core::sc_trace(tf, trg_i, "Gost34112018_Hw.trg");
    sc_core::sc_trace(tf, reset_i, "Gost34112018_Hw.reset");
    sc_core::sc_trace(tf, block_i, "Gost34112018_Hw.block");
    sc_core::sc_trace(tf, block_size_i, "Gost34112018_Hw.block_size");
    sc_core::sc_trace(tf, hash_size_i, "Gost34112018_Hw.hash_size");

    cl_->trace(tf);
    st_->trace(tf);
    gn_->trace(tf);
    sl_->trace(tf);
    p_->trace(tf);
}

u512 Gost34112018_Hw::read_hash() const
{
    return (*hash_o)->read();
}

Gost34112018_Hw::State Gost34112018_Hw::read_state() const
{
    return static_cast<State>((*state_o)->read().to_int());
}

}
