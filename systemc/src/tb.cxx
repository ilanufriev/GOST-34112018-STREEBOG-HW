#include <gost34112018_hw.hxx>
#include <common.hxx>
#include <datatypes.hxx>
#include <sysc/kernel/sc_wait.h>
#include <utils.hxx>
#include <stage.hxx>

#include <boost/program_options.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <ostream>
#include <iomanip>
#include <sstream>
#include <iostream>

using streebog_hw::BLOCK_SIZE;

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

int64_t g_clock_counter = 0;

void wait_clk(int cycles, sc_core::sc_signal<bool> &clk)
{
    for (int i = 0; i < cycles; ++i)
    {
        clk = true;
        sc_core::sc_start(static_cast<int>(streebog_hw::CLOCK_CYCLE_NS / 2), sc_core::SC_NS);
        clk = false;
        sc_core::sc_start(static_cast<int>(streebog_hw::CLOCK_CYCLE_NS / 2), sc_core::SC_NS);
        g_clock_counter += 1;
    }
}

#define ADVANCE_WHILE(__condition) while (__condition) sc_core::sc_start(10, sc_core::SC_NS)
#define ADVANCE_WHILE_CLK(__condition, __clk) while (__condition) wait_clk(1, __clk)

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

    streebog_hw::Gost34112018_Hw gost("GOSTHW");

    sc_core::sc_signal<bool> trg;
    sc_core::sc_signal<bool> reset;
    sc_core::sc_signal<bool> hash_size;
    sc_core::sc_signal<bool> clk;
    sc_core::sc_signal<streebog_hw::u512> block;
    sc_core::sc_signal<streebog_hw::u8>   block_size;

    sc_core::sc_signal<streebog_hw::u512> hash;
    sc_core::sc_signal<streebog_hw::Gost34112018_Hw::ScState> state;

    gost.trg_i.bind(trg);
    gost.reset_i.bind(reset);
    gost.hash_size_i.bind(hash_size);
    gost.clk_i.bind(clk);
    gost.block_i.bind(block);
    gost.block_size_i.bind(block_size);
    gost.hash_o.bind(hash);
    gost.state_o.bind(state);

#ifdef __ENABLE_WAVEFORM_TRACING__
    sc_core::sc_trace_file *tf = sc_core::sc_create_vcd_trace_file("Gost34112018_Hw_trace");
    tf->set_time_unit(1, sc_core::SC_NS);

    gost.trace(tf);
#endif

    auto hash_block = [&trg, &clk, &block, &block_size, &hash_size, &gost]
    (const unsigned char *in_block, const uint8_t in_block_size, const bool in_hash_size) {
        block = streebog_hw::bytes_to_sc_uint512(in_block, in_block_size);
        block_size = in_block_size;
        hash_size = in_hash_size;

        trg.write(1);

        wait_clk(1, clk);

        trg.write(0);

        DEBUG_OUT << "block = " << block.read().to_string(sc_dt::SC_HEX) << std::endl;
        DEBUG_OUT << "block_size = " << block_size.read() << std::endl;
        DEBUG_OUT << "hash_size = " << hash_size.read() << std::endl;

        DEBUG_OUT << "Waiting for ready or done\n";
        ADVANCE_WHILE_CLK(gost.state_o->read() != streebog_hw::Gost34112018_Hw::State::READY &&
                          gost.state_o->read() != streebog_hw::Gost34112018_Hw::State::DONE,
                          clk);
    };

    sc_core::sc_start(0, sc_core::SC_NS);

    wait_clk(4, clk);

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
    std::array<unsigned char, BLOCK_SIZE> hash_bytes_full;
    streebog_hw::sc_uint512_to_bytes(hash_bytes_full.data(), hash_bytes_full.size(), gost.hash_o->read());
    
    std::vector<unsigned char> hash_bytes_result(
        hash_bytes_full.rbegin(),
        hash_bytes_full.rbegin() + hash_size_int
    );

    std::ostringstream hash_os;
    hash_os << std::hex << std::setw(2) << std::setfill('0');

    if (args.count("big-endian"))
    {
        for (int i = 0; i < (hash_size_int / 8); i++)
        {
            hash_os << std::hex << std::setw(2) << (static_cast<int>(hash_bytes_result[i]) & 0xFF);
        }
        hash_str = hash_os.str();
    }
    else
    {
        for (int i = 0; i < (hash_size_int / 8); i++)
        {
            hash_os << std::hex << std::setw(2) << std::setfill('0') << (static_cast<int>(hash_bytes_result[(hash_size_int / 8) - 1 - i]) & 0xFF);
        }
        hash_str = hash_os.str();
    }

    DEBUG_OUT << g_clock_counter << " clocks passed\n";

    std::cout << hash_str << std::endl;

    trg.write(1);

    wait_clk(1, clk);

    trg.write(0);

    ADVANCE_WHILE_CLK(gost.state_o->read() != streebog_hw::Gost34112018_Hw::State::CLEAR,
                      clk);

#ifdef __ENABLE_WAVEFORM_TRACING__
    sc_core::sc_close_vcd_trace_file(tf);
#endif

    return 0;
}
