#include <boost/program_options.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <common.hxx>
#include <control_logic.hxx>
#include <datatypes.hxx>
#include <ostream>
#include <utils.hxx>
#include <iomanip>
#include <sstream>
#include <stage.hxx>
#include <iostream>

constexpr int32_t BLOCK_SIZE = 64;

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

template <std::size_t N>
std::string array_to_hex_string(const std::array<unsigned char, N> &arr, const std::size_t memb)
{
    std::ostringstream os;
    os << "[ ";
    for (std::size_t i = 0; i < memb; ++i) {
        os << streebog_hw::byte_to_hex_string(arr[i]);
        if (i < memb - 1) {
            os << ", "; // Add a comma between elements
        }
    }
    os << " ]";
    return os.str();
}

namespace po = boost::program_options;

#define ADVANCE_WHILE(__condition) while (__condition) sc_core::sc_start(10, sc_core::SC_NS)

int sc_main(int argc, char **argv)
{
    static std::array<unsigned char, 65536> buffer;

    po::options_description desc("Allowed options");

    desc.add_options()
        ("help", "produce help message")
        ("hash-size,s", po::value<int>(), "Size of the produced hash (512 or 256, 512 is default)")
        ("input-file,i", po::value<std::vector<std::string>>(), "A file from which data will be read")
        ("big-endian", "Print hash in big-endian format (little-endian is default)")
    ;

    po::variables_map args;
    po::store(po::parse_command_line(argc, argv, desc), args);
    po::notify(args);

    std::string input_file;
    int hash_size_int = 512;
    bool hash_size_opt = 0;

    if (args.count("help"))
    {
        std::cout << desc << std::endl;
        return 0;
    }

    if (args.count("hash-size") != 0)
    {
        hash_size_int = args.at("hash-size").as<int>();
        if (hash_size_int != 256 && hash_size_int != 512)
        {
            std::cout << "Wrong hash size. Choose either 512 or 256.\n";
            return 1;
        }

        hash_size_opt = hash_size_int == 256 ? 1 : 0;
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

    auto hash_block = [&start, &ack, &block, &block_size, &hash_size, &cl]
    (const unsigned char *in_block, const uint8_t in_block_size, const bool in_hash_size) {
        block = streebog_hw::bytes_to_sc_uint512(in_block, in_block_size);
        block_size = in_block_size;
        hash_size = in_hash_size;

        start = 1;

        DEBUG_OUT << "Waiting for busy\n";
        ADVANCE_WHILE(cl.state_o->read() != streebog_hw::ControlLogic::State::BUSY);

        DEBUG_OUT << "block = " << block.read().to_string(sc_dt::SC_HEX) << std::endl;
        DEBUG_OUT << "block_size = " << block_size.read() << std::endl;
        DEBUG_OUT << "hash_size = " << hash_size.read() << std::endl;

        start = 0;
        DEBUG_OUT << "Waiting for ready or done\n";
        ADVANCE_WHILE(cl.state_o->read() != streebog_hw::ControlLogic::State::READY &&
                      cl.state_o->read() != streebog_hw::ControlLogic::State::DONE);
    };

    sc_core::sc_start(0, sc_core::SC_NS);

    int32_t rc = 0;
    int32_t last_block_size = 64;

    while (!fin.eof())
    {
        DEBUG_OUT << "Read started\n";
        
        // I know that this is very dirty, but the entire library
        // libgost34112018 uses unsigned values since they represent 
        // abstract bits. The library also makes use of the overflow
        // mechanism of unsigned numbers to implement 2^512 modulo 
        // addition. So I have to do this cast to just read bytes
        // into a buffer of **unsigned** chars, while the fin.read()
        // only supports usual (signed) chars. God forgive me.
        fin.read(reinterpret_cast<char *>(buffer.data() + rc), buffer.size() - rc);
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

    DEBUG_OUT << array_to_hex_string(buffer, 96) << std::endl;

    // we hit feof, but rc is not empty
    if (rc != 0)
    {
        int i;
        for (i = 0; (i + BLOCK_SIZE) < rc; i += BLOCK_SIZE)
        {
            DEBUG_OUT << "Hashing block #" << (i + 1) << std::endl;
            hash_block(buffer.data() + i, BLOCK_SIZE, hash_size_opt);
            last_block_size = BLOCK_SIZE;
        }

        DEBUG_OUT << "Hashing the last block" << std::endl;

        // Hash the rest of the message
        hash_block(buffer.data() + i, rc - i, hash_size_opt);
        last_block_size = rc - i;
    }

    // To correctly end the hashing, we need to send zero-length
    // if the data size is divisible by 64
    if (last_block_size == BLOCK_SIZE)
    {
        DEBUG_OUT << "Ending the hashing" << std::endl;
        hash_block(nullptr, 0, hash_size_opt);
    }

    // Format the input: print them in the right order
    std::string hash_str;
    std::array<unsigned char, BLOCK_SIZE> hash_bytes;
    streebog_hw::sc_uint512_to_bytes(hash_bytes.data(), hash_bytes.size(), cl.hash_o->read());
    std::ostringstream hash_os;
    hash_os << std::hex << std::setw(2) << std::setfill('0');

    if (args.count("big-endian"))
    {
        for (int i = 0; i < (hash_size_int / 8); i++)
        {
            hash_os << std::hex << std::setw(2) << std::setfill('0') << (static_cast<int>(hash_bytes[(hash_size_int / 8) - 1 - i]) & 0xFF);
        }
        hash_str = hash_os.str();
    }
    else
    {
        std::ostringstream os;
        for (int i = 0; i < (hash_size_int / 8); i++)
        {
            hash_os << std::hex << std::setw(2) << (static_cast<int>(hash_bytes[i]) & 0xFF);
        }
        hash_str = hash_os.str();
    }

    std::cout << hash_str << std::endl;
    ack.write(1);

    ADVANCE_WHILE(cl.state_o->read() != streebog_hw::ControlLogic::State::CLEAR);

    return 0;
}
