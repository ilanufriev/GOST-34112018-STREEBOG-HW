#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <datatypes.hxx>

#define METHOD_NOT_IMPLEMENTED \
    std::cerr << "Method " << __func__ << " was called but is not implemented." << std::endl

#define AUTONAME(__obj) \
    __obj(#__obj)

#define WAIT_WHILE(__condition) \
    while(__condition) { streebog_hw::wait_clk(1); }

#define __ENABLE_DEBUG_MESSAGES__
#ifdef __ENABLE_DEBUG_MESSAGES__

#define DEBUG_OUT_ENABLED 1
    #define DEBUG_OUT \
        if (DEBUG_OUT_ENABLED) std::cerr << "[DEBUG] from " << __PRETTY_FUNCTION__ << ", line " << __LINE__ << ": "
#else
    #define DEBUG_OUT \
        if (0) std::cerr
#endif // __ENABLE_DEBUG_MESSAGES__

namespace streebog_hw
{

u512 bytes_to_sc_uint512(const unsigned char *bytes, const int32_t block_size);

std::string byte_to_hex_string(const unsigned char bytes, const std::string prefix = "");

std::string bytes_to_hex_string(const unsigned char *bytes, const int64_t bytes_size, const std::string prefix = "");

void sc_uint512_to_bytes(unsigned char *bytes, const std::size_t bytes_size, u512 from);

}

#endif // __UTILS_HPP__
