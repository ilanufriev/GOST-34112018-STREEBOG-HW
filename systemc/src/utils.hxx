#ifndef __UTILS_HPP__
#define __UTILS_HPP__

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
    if (DEBUG_OUT_ENABLED) std::cerr << "[DEBUG] from " << __func__ << ", line " << __LINE__ << ": "
#else
    if (0) std::cerr
#endif // __ENABLE_DEBUG_MESSAGES__

#endif // __UTILS_HPP__
