#include <boost/program_options.hpp>
#include <common.hxx>
#include <control_logic.hxx>
#include <datatypes.hxx>
#include <fstream>
#include <stage.hxx>
#include <utils.hxx>

#include <iostream>

constexpr int32_t BLOCK_SIZE = 64;

streebog_hw::u512 bytes_to_sc_uint512(const char *bytes, const int32_t block_size)
{
    streebog_hw::u512 result = 0;

    for (int i = block_size - 1; i >= 0; i--)
    {
        result <<= 8;
        result  |= bytes[i];
    }

    return result;
}

template <typename T, std::size_t N>
std::ostream& operator<<(std::ostream& os, const std::array<T, N>& arr) {
    os << "[";
    for (std::size_t i = 0; i < N; ++i) {
        os << arr[i];
        if (i < N - 1) {
            os << ", "; // Add a comma between elements
        }
    }
    os << "]";
    return os;
}

namespace po = boost::program_options; 

#define ADVANCE_WHILE(__condition) sc_core::sc_start(10, sc_core::SC_NS)

int sc_main(int argc, char **argv)
{
    static std::array<char, 65536> buffer;

    po::options_description desc("Allowed options");

    desc.add_options()
        ("help", "produce help message")
        ("hash-size,s", po::value<int>(), "Size of the produced hash (512 or 256, 512 is default)")
        ("input-file,i", po::value<std::vector<std::string>>(), "A file from which data will be read")
    ;

    po::variables_map args;
    po::store(po::parse_command_line(argc, argv, desc), args);
    po::notify(args);
 
    std::string input_file;
    bool hash_size_opt = 0;

    if (args.count("hash-size") != 0)
    {
        int tmp = args.at("has-size").as<int>();
        if (tmp != 256 && tmp != 512)
        {
            std::cout << "Wrong hash size. Choose either 512 or 256.\n";
            return 1;
        }

        hash_size_opt = tmp == 256 ? 1 : 0;
    }

    if (args.count("input-file") == 0)
    {
        input_file = "/dev/stdin";
    }
    else
    {
        input_file = args.at("input-file").as<std::vector<std::string>>().at(0);
    }
    
    std::ifstream fin{input_file, std::ios::binary};
    
    if (!fin.is_open())
    {
        std::cout << "Could not open the input file \"" << input_file << "\"\n";
        return 1;
    }

    streebog_hw::ControlLogic cl("cl");
    streebog_hw::Stage st("st");

    sc_core::sc_signal<bool> start;
    sc_core::sc_signal<bool> reset;
    sc_core::sc_signal<bool> hash_size;
    sc_core::sc_signal<bool> ack;
    sc_core::sc_signal<streebog_hw::u512> block;
    sc_core::sc_signal<streebog_hw::u8>   block_size;

    cl.start_i(start);
    cl.reset_i(reset);
    cl.hash_size_i(hash_size);
    cl.ack_i(ack);
    cl.block_i(block);
    cl.block_size_i(block_size);

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
    
    auto hash_block = [&start, &ack, &block, &block_size, &hash_size, &cl](const char *in_block, const int32_t in_block_size, const bool in_hash_size) {
        block = bytes_to_sc_uint512(in_block, in_block_size);
        block_size = in_block_size;
        hash_size = in_hash_size;
        
        start = 1;

        DEBUG_OUT << "Waiting for busy\n";
        ADVANCE_WHILE(cl.state_o->read() != streebog_hw::ControlLogic::State::BUSY);

        DEBUG_OUT << "block = " << block.read().to_string(sc_dt::SC_HEX) << std::endl;
        DEBUG_OUT << "block_size = " << block_size << std::endl;
        DEBUG_OUT << "hash_size = " << hash_size << std::endl;

        start = 0;
        DEBUG_OUT << "Waiting for ready or done\n";
        ADVANCE_WHILE(cl.state_o->read() != streebog_hw::ControlLogic::State::READY ||
                      cl.state_o->read() != streebog_hw::ControlLogic::State::DONE);
    };

    sc_core::sc_start(0, sc_core::SC_NS);

    int32_t rc = 0;
    int32_t last_block_size = 0;
    
    DEBUG_OUT << "Going for a file read\n";
    while (!fin.eof())
    {
        DEBUG_OUT << "Read started\n";
        fin.read(buffer.data() + rc, buffer.size() - rc);
        rc = fin.gcount();
        

        if (rc < buffer.size() || fin.eof())
            continue;
        
        DEBUG_OUT << "Read rc = " << rc << " bytes\n";
        for (int i = 0; i < rc; i += BLOCK_SIZE)
        {
            DEBUG_OUT << "Hashing block " << i << std::endl;
            hash_block(buffer.data() + i, BLOCK_SIZE, hash_size_opt);
            last_block_size = BLOCK_SIZE;
        }
        rc = 0;
    }

    // we hit feof, but rc is not empty
    if (rc != 0)
    {
        int i;
        for (i = 0; (i + BLOCK_SIZE) < rc; i += BLOCK_SIZE)
        {
            hash_block(buffer.data() + i, BLOCK_SIZE, hash_size_opt);
            last_block_size = BLOCK_SIZE;
        }

        // Hash the rest of the message
        hash_block(buffer.data() + i, rc - i, hash_size_opt);
        last_block_size = rc - i;
    }

    if (last_block_size == BLOCK_SIZE)
    {
        DEBUG_OUT << "Ending the hashing" << std::endl;
        hash_block(nullptr, 0, hash_size_opt);
    }
    
    std::cout << "hash = " << cl.hash_o->read() << std::endl;

    ack.write(1);

    ADVANCE_WHILE(cl.state_o.read() != ControlLogic::State::CLEAR);

    return 0;
}
