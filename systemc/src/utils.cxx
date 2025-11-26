#include <utils.hxx>
#include <sstream>
#include <iomanip>

namespace streebog_hw
{

u512 bytes_to_sc_uint512(const unsigned char *bytes, const int32_t block_size)
{
    u512 result = 0;

    for (int i = block_size - 1; i >= 0; i--)
    {
        result <<= 8;
        result  |= (bytes[i] & 0xFF);
    }

    return result;
}

void sc_uint512_to_bytes(unsigned char *bytes, const std::size_t bytes_size, u512 from)
{
    for (int i = 0; i < bytes_size; i++)
    {
        bytes[i] = (from.to_int() & 0xFF);
        from >>= 8;
    }
}

std::string bytes_to_hex_string(const unsigned char *bytes, const int64_t bytes_size, const std::string prefix)
{
    std::ostringstream os;
    os << prefix;
    for (int64_t i = 0; i < bytes_size; i++)
    {
        os << byte_to_hex_string(bytes[i]);
    }
    return os.str();
}

std::string byte_to_hex_string(const unsigned char num, const std::string prefix)
{
    std::ostringstream os;
    os << prefix;
    os << std::hex << std::setw(2) << std::setfill('0') << (+num & 0xFF);
    return os.str();
}

}
