#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace ddm
{

enum class status_t
{
    PENDING,
    SUBMITTED,
    COMPLETED,
    FAILED
};

template <typename T>
concept Event = requires(T t) {
    {
        t.to_string()
    } -> std::convertible_to<std::string>;
    {
        t.status()
    } -> std::convertible_to<status_t>;
};

/**
 * @brief High-level events
 */

struct event_t
{
    status_t            status_ {};
    virtual status_t    status() { return status_; };
    virtual std::string to_string() const = 0;
    virtual ~event_t()                    = default;
};

typedef std::shared_ptr<event_t> event_ptr_t;

enum device_event_type_t
{
    DEVICE_EVENT_TYPE_BEGIN = 0,
    INITIAL_READ,
    INITIAL_PROGRAM,
    INITIAL_ERASE,
    SET_ADDRESS,
    DO_TRANSFER,
    DO_READ,
    DO_PROGRAM,
    DO_ERASE,
    END_TRANSFER,
    TIME_WAIT,
    MAX_DEVICE_EVENT_TYPES
};

struct time_wait_event_t : public event_t
{
    static constexpr int id = device_event_type_t::TIME_WAIT;

    std::uint32_t ms {};
    explicit time_wait_event_t(std::uint32_t ms)
        : ms { ms }
    {
    }
    virtual std::string to_string() const override;
};

struct _pointer_specify_event_t : public event_t
{
    const unsigned char* buf { nullptr };
    explicit _pointer_specify_event_t(const unsigned char* buf)
        : buf { buf }
    {
    }
};

struct initial_read_event_t : public _pointer_specify_event_t
{
    static constexpr int id = device_event_type_t::INITIAL_READ;

    using _pointer_specify_event_t::_pointer_specify_event_t;
    virtual std::string to_string() const override;
};

struct initial_program_event_t : public _pointer_specify_event_t
{
    static constexpr int id = device_event_type_t::INITIAL_PROGRAM;

    using _pointer_specify_event_t::_pointer_specify_event_t;
    virtual std::string to_string() const override;
};

struct _empty_body_event_t : public event_t
{
    virtual std::string name() const = 0;
    virtual std::string to_string() const override { return name() + "()"; }
};

struct initial_erase_event_t : public _empty_body_event_t
{
    static constexpr int id = device_event_type_t::INITIAL_ERASE;

    virtual std::string name() const override { return "INITIAL_ERASE"; }
};

struct set_address_event_t : public event_t
{
    static constexpr int id = device_event_type_t::SET_ADDRESS;

    enum class address_type_t
    {
        Block,
        Page,
        Byte,
    };
    address_type_t addr_type {};
    std::uint8_t   addr {};
    set_address_event_t(address_type_t type, std::uint8_t addr)
        : addr_type { type }
        , addr { addr }
    {
    }
    virtual std::string to_string() const override;
};

using address_type_t = set_address_event_t::address_type_t;

struct do_read_event_t : public _empty_body_event_t
{
    static constexpr int id = device_event_type_t::DO_READ;

    virtual std::string name() const override { return "DO_READ"; }
};

struct do_program_event_t : public _empty_body_event_t
{
    static constexpr int id = device_event_type_t::DO_PROGRAM;

    virtual std::string name() const override { return "DO_PROGRAM"; }
};

struct do_erase_event_t : public _empty_body_event_t
{
    static constexpr int id = device_event_type_t::DO_ERASE;

    virtual std::string name() const override { return "DO_ERASE"; }
};

struct do_transfer_event_t : public _empty_body_event_t
{
    static constexpr int id = device_event_type_t::DO_TRANSFER;

    virtual std::string name() const override { return "DO_TRANSFER"; }
};

struct end_transfer_event_t : public _empty_body_event_t
{
    static constexpr int id = device_event_type_t::END_TRANSFER;

    virtual std::string name() const override { return "END_TRANSFER"; }
};

using l2_event_t = std::variant<
    time_wait_event_t,
    initial_read_event_t,
    initial_program_event_t,
    initial_erase_event_t,
    set_address_event_t,
    do_read_event_t,
    do_program_event_t,
    do_erase_event_t,
    do_transfer_event_t,
    end_transfer_event_t>;

/**
 * @brief Intermediate-level events
 */

struct cmd_event_t : public event_t
{
    uint8_t opcode {};
    explicit cmd_event_t(uint8_t opcode)
        : opcode { opcode }
    {
    }
    virtual std::string to_string() const override;
};

struct addr_event_t : public event_t
{
    std::vector<uint8_t> addr {};
    explicit addr_event_t(const std::vector<uint8_t>& addr)
        : addr { addr }
    {
    }
    virtual std::string to_string() const override;
};

struct data_in_event_t : public event_t
{
    size_t         size {};
    const uint8_t* buf { nullptr };
    data_in_event_t(size_t size, const uint8_t* buf)
        : size { size }
        , buf { buf }
    {
    }
    virtual std::string to_string() const override;
};

struct data_out_event_t : public event_t
{
    size_t   size {};
    uint8_t* buf { nullptr };
    data_out_event_t(size_t size, uint8_t* buf)
        : size { size }
        , buf { buf }
    {
    }
    virtual std::string to_string() const override;
};

using l1_event = std::variant<time_wait_event_t, cmd_event_t, addr_event_t, data_in_event_t, data_out_event_t>;

struct read_register_event_t : public event_t
{
    size_t offset;
    explicit read_register_event_t(size_t offset)
        : offset { offset }
    {
    }
    virtual std::string to_string() const override;
};

struct write_register_event_t : public event_t
{
    size_t offset;
    size_t value;
    explicit write_register_event_t(size_t offset, size_t value)
        : offset { offset }
        , value { value }
    {
    }
    virtual std::string to_string() const override;
};

using l0_event = std::variant<time_wait_event_t, read_register_event_t, write_register_event_t>;

} // namespace ddm
