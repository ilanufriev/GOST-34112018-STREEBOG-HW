// Copyright 2025, Anufriev Ilia, anufriewwi@rambler.ru
// SPDX-License-Identifier: BSD-3-Clause-No-Military-License OR GPL-3.0-or-later

#ifndef __COMMON_HXX__
#define __COMMON_HXX__

#include "datatypes.hxx"

namespace streebog_hw
{

/**
    @brief      S-box, as defined in the chapter 5.1 of The Standard. It is used for
                S-transformation.
 */
extern const gost_u8 PI[256];

/**
    @brief      A byte shuffle table, as defined in the ch. 5.3 of The Standard. It is
                used for the P transformation.
 */
extern const gost_u8 TAU[64];

enum
{
    A_SIZE = 64,
    C_SIZE = 12,
};

/**
    @brief      Matrix A, as defined in the ch. 5.4 of The Standard. It is used for the L
                transformation.
*/
extern const gost_u64 A[64];

/**
    @brief      Iteration constants, that are used in the encryption function of the
                algorithm.
    @note       All constants are Little-Endian unsigned 512-bit numbers, which means that,
                for example, C1.bytes[0] is LSB and C1.bytes[63] is MSB.
 */
extern const gost_u512 * const C[];

/**
    @brief      Initialization vector for the 256-bit long digest, as defined in ch. 5.1
                of The Standard.
*/
extern const gost_u512 INIT_VECTOR_256;

/**
    @brief      Initialization vector for the 512-bit long digest, as defined in ch. 5.1
                of The Standard.
*/
extern const gost_u512 INIT_VECTOR_512;

extern const gost_u512 ZERO_VECTOR_512;

}
#endif // __COMMON_HXX__
