#include <cstddef>

namespace ddm
{

enum operation_t
{
    OP_INVALID,
    OP_READ,
    OP_PROGRAM,
    OP_ERASE
};

enum action_t
{
    ACT_ADDR,
    ACT_READ_NEXT,
    ACT_WRITE_NEXT,
    ACT_ERASE_NEXT,
    ACT_TRANSFER,
};

struct context_t
{
    operation_t operation {};
    size_t      block{}, page{}, byte {};
    size_t      cursor {};
    action_t    action {};
    size_t      action_ts {};
};

}
