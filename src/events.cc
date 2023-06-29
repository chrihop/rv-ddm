#include <events.h>

namespace ddm
{

std::string
    time_wait_event_t::to_string() const
{
    return "TIME_WAIT(" + std::to_string(ms) + ")";
}

std::string
initial_read_event_t::to_string() const
{
    return "INITIAL_READ(buf=" + std::to_string((size_t)buf) + ")";
}

std::string
initial_program_event_t::to_string() const
{
    return "INITIAL_PROGRAM(buf=" + std::to_string((size_t)buf) + ")";
}

static std::string to_string(address_type_t type)
{
    switch(type)
    {
    case address_type_t::Page: return "Page";
    case address_type_t::Block: return "Block";
    case address_type_t::Byte: return "Byte";
    }
    return "Unknown";
}


std::string
set_address_event_t::to_string() const
{
    return "SET_ADDRESS(type=" + ddm::to_string(addr_type)
        + ", addr=" + std::to_string(addr) + ")";
}

std::string
cmd_event_t::to_string() const
{
    return "CMD(opcode=" + std::to_string(opcode) + ")";
}

std::string
addr_event_t::to_string() const
{
    std::string str = "ADDR(";
    for (auto& b : addr)
    {
        str += std::to_string(b) + ", ";
    }
    str += ")";
    return str;
}

std::string
data_in_event_t::to_string() const
{
    std::string str = "DATA_IN(size=" + std::to_string(size)
        + ", buf=" + std::to_string((size_t)buf) + ")";
    return str;
}

std::string
data_out_event_t::to_string() const
{
    std::string str = "DATA_OUT(size=" + std::to_string(size)
        + ", buf=" + std::to_string((size_t)buf) + ")";
    return str;
}

std::string
read_register_event_t::to_string() const
{
    std::string str = "READ_REGISTER(offset=" + std::to_string(offset) + ")";
    return str;
}

std::string
write_register_event_t::to_string() const
{
    std::string str = "WRITE_REGISTER(offset=" + std::to_string(offset) + ", value=" + std::to_string(value) + ")";
    return str;
}

} /* namespace ddm */
