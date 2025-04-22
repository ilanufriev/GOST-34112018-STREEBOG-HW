#include <datatypes.hxx>
#include <iostream>
#include <control_logic.hxx>
#include <stage.hxx>
#include <boost/program_options.hpp>

namespace po = boost::program_options; 

int sc_main(int argc, char **argv)
{
    po::options_description desc("Allowed options");

    desc.add_options()
        ("help", "produce help message")
        ("hash-size,s", po::value<int>(), "Size of the produced hash (512 or 256, 512 is default)")
        ("input-file,i", po::value<std::vector<std::string>>(), "A file from which data will be read")
    ;

    streebog_hw::ControlLogic cl("cl");
    streebog_hw::Stage st("st");

    sc_core::sc_signal<bool> start;
    sc_core::sc_signal<bool> reset;
    sc_core::sc_signal<bool> hash_size;
    sc_core::sc_signal<bool> ack;
    sc_core::sc_signal<streebog_hw::u512> block;
    sc_core::sc_signal<streebog_hw::u8>   block_size;

    st.block_i(cl.st_block_o);
    st.block_size_i(cl.st_block_size_o);
    st.sigma_i(cl.sigma_o);
    st.n_i(cl.n_o);
    st.h_i(cl.h_o);
    st.ack_i(cl.st_ack_o);
    st.start_i(cl.st_start_o);
    st.sel_i(cl.st_sel_o);
    
    cl.sigma_nx_i(st.sigma_nx_o);
    cl.n_nx_i(st.n_nx_o);
    cl.h_nx_i(st.h_nx_o);
    cl.st_state_i(st.state_o);

    return 0;
}
