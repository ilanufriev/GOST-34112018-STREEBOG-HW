#ifndef __PAYLOADS_HXX__
#define __PAYLOADS_HXX__

#include <systemc>
#include <datatypes.hxx>

namespace streebog_hw
{

struct ControlLogicToStage2Data
{
    gost_algctx ctx;
    gost_u512   block;
};

struct ControlLogicToStage3Data
{
    gost_algctx ctx;
    gost_u512   block;
    gost_u64    block_size;
};

}

#endif // __PAYLOADS_HXX__
