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

    // ControlLogic to outside I/O
    cl_->trg_i.bind(trg_i);
    cl_->reset_i.bind(reset_i);
    cl_->block_i.bind(block_i);
    cl_->block_size_i.bind(block_size_i);
    cl_->hash_size_i.bind(hash_size_i);
    cl_->clk_i.bind(clk_i);

    cl_->state_o.bind(state_o);
    cl_->hash_o.bind(hash_o);
 
    // ControlLogic to Stage I/O
    cl_->sigma_nx_i.bind(cl_st_sigma_nx_);
    cl_->n_nx_i.bind(cl_st_n_nx_);
    cl_->h_nx_i.bind(cl_st_h_nx_);
    cl_->st_state_i.bind(cl_st_state_);

    cl_->st_block_o.bind(cl_st_block_);
    cl_->st_block_size_o.bind(cl_st_block_size_);
    cl_->sigma_o.bind(cl_st_sigma_);
    cl_->n_o.bind(cl_st_n_);
    cl_->h_o.bind(cl_st_h_);
    cl_->st_trg_o.bind(cl_st_trg_);

    st_->trg_i.bind(cl_st_trg_);
    st_->block_i.bind(cl_st_block_);
    st_->block_size_i.bind(cl_st_block_size_);
    st_->sigma_i.bind(cl_st_sigma_);
    st_->n_i.bind(cl_st_n_);
    st_->h_i.bind(cl_st_h_);
    st_->clk_i.bind(clk_i);
    
    // Stage to Gn I/O
    st_->sigma_nx_o.bind(cl_st_sigma_nx_);
    st_->n_nx_o.bind(cl_st_n_nx_);
    st_->h_nx_o.bind(cl_st_h_nx_);
    st_->state_o.bind(cl_st_state_);

    st_->g_n_result_i.bind(st_g_n_result_);
    st_->g_n_state_i.bind(st_g_n_state_);
    st_->g_n_m_o.bind(st_g_n_m_);
    st_->g_n_n_o.bind(st_g_n_n_);
    st_->g_n_h_o.bind(st_g_n_h_);
    st_->g_n_trg_o.bind(st_g_n_trg_);

    gn_->m_i.bind(st_g_n_m_);
    gn_->n_i.bind(st_g_n_n_);
    gn_->h_i.bind(st_g_n_h_);
    gn_->trg_i.bind(st_g_n_trg_);
    gn_->clk_i.bind(clk_i);

    gn_->result_o.bind(st_g_n_result_);
    gn_->state_o.bind(st_g_n_state_);

    // Gn to P and SL I/O
    gn_->sl_tr_result_i.bind(g_n_sl_tr_result_);
    gn_->p_tr_result_i.bind(g_n_p_tr_result_);

    gn_->sl_tr_a_o.bind(g_n_sl_tr_a_);
    gn_->p_tr_a_o.bind(g_n_p_tr_a_);

    sl_->a_i.bind(g_n_sl_tr_a_);
    sl_->result_o.bind(g_n_sl_tr_result_);

    p_->a_i.bind(g_n_p_tr_a_);
    p_->result_o.bind(g_n_p_tr_result_);

    SC_THREAD(thread);
}

void Gost34112018_Hw::thread()
{
}

void Gost34112018_Hw::trace(sc_core::sc_trace_file *tf)
{
    sc_core::sc_trace(tf, clk_i, clk_i.name());
    sc_core::sc_trace(tf, trg_i, trg_i.name());
    sc_core::sc_trace(tf, reset_i, reset_i.name());
    sc_core::sc_trace(tf, block_i, block_i.name());
    sc_core::sc_trace(tf, block_size_i, block_size_i.name());
    sc_core::sc_trace(tf, hash_size_i, hash_size_i.name());

    cl_->trace(tf);
    st_->trace(tf);
    gn_->trace(tf);
    sl_->trace(tf);
    p_->trace(tf);
}

}
