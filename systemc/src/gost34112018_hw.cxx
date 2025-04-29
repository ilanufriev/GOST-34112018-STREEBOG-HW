#include "control_logic.hxx"
#include "utils.hxx"
#include <gost34112018_hw.hxx>
#include <iterator>
#include <memory>
#include <sysc/kernel/sc_module.h>
#include <sysc/kernel/sc_module_name.h>

namespace streebog_hw
{

Gost34112018_Hw::Gost34112018_Hw(sc_core::sc_module_name const &name)
{
    cl_ = std::make_unique<ControlLogic>("ControlLogic");
    st_ = std::make_unique<Stage>("Stage");
    gn_ = std::make_unique<Gn>("Gn");
    sl_ = std::make_unique<SLTransform>("SL");
    p_  = std::make_unique<PTransform>("P");

    cl_->start_i.bind(start_i);
    cl_->reset_i.bind(reset_i);
    cl_->ack_i.bind(ack_i);
    cl_->block_i.bind(block_i);
    cl_->block_size_i.bind(block_size_i);
    cl_->hash_size_i.bind(hash_size_i);

    hash_o = &cl_->hash_o;
    state_o = &cl_->state_o;

    cl_->sigma_nx_i.bind(st_->sigma_nx_o);
    cl_->n_nx_i.bind(st_->n_nx_o);
    cl_->h_nx_i.bind(st_->h_nx_o);
    cl_->st_state_i.bind(st_->state_o);

    st_->ack_i.bind(cl_->st_ack_o);
    st_->start_i.bind(cl_->st_start_o);
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
    gn_->start_i.bind(st_->g_n_start_o);
    gn_->ack_i.bind(st_->g_n_ack_o);

    gn_->sl_tr_result_i.bind(sl_->result_o);
    gn_->p_tr_result_i.bind(p_->result_o);

    sl_->a_i.bind(gn_->sl_tr_a_o);
    p_->a_i.bind(gn_->p_tr_a_o);

    SC_THREAD(thread);
}

void Gost34112018_Hw::thread()
{
}

u512 Gost34112018_Hw::read_hash() const
{
    return (*hash_o)->read();
}

Gost34112018_Hw::State Gost34112018_Hw::read_state() const
{
    return static_cast<State>((*state_o)->read());
}

}
