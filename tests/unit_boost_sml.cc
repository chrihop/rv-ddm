#include <context.h>
#include <cstdio>
#include <events.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/sml.hpp>
#include <boost/sml/utility/dispatch_table.hpp>

namespace sml = boost::sml;

class BoostSMLUnitTest : public ::testing::Test
{
protected:
    void SetUp() override { }

    void TearDown() override { }
};

struct device_context_t : public ddm::context_t
{
    std::vector<std::string> trace;
};

template <ddm::Event E> struct guard
{
    bool operator()(const E& e) const
    {
        printf("guard: %s\n", e.to_string().c_str());
        return true;
    }
};

template <ddm::Event E> struct action
{
    void operator()(const E& e) const { printf("action: %s\n", e.to_string().c_str()); }
};

#define TRANS(FROM, EVENT, TO) FROM##_s + event<ddm::EVENT>[guard<ddm::EVENT> {}] / action<ddm::EVENT> {} = TO##_s

template <> auto sml::state<class X_s> = sml::X;

TEST_F(BoostSMLUnitTest, make_transition)
{
    /* InitialRead -> SetAddr -> Transfer -> DoRead -> Transfer -> X */
    device_context_t ctx {};

    struct DeviceTransition
    {
        auto operator()() const
        {
            using namespace sml;
            return make_transition_table(*TRANS("idle", initial_read_event_t, "wait_for_addr"),
                TRANS("wait_for_addr", set_address_event_t, "wait_for_addr"),
                TRANS("wait_for_addr", do_transfer_event_t, "wait_for_action"),
                TRANS("wait_for_action", do_read_event_t, "transfer"),
                TRANS("transfer", do_transfer_event_t, "transfer"), TRANS("transfer", end_transfer_event_t, "X"));
        }
    };

    DeviceTransition          trans {};
    sml::sm<DeviceTransition> sm { trans, ctx };

    using namespace sml;

    uint8_t* buf = nullptr;
    EXPECT_TRUE(sm.is("idle"_s));

    std::vector<ddm::l2_event_t> events { ddm::initial_read_event_t(nullptr),
        ddm::set_address_event_t(ddm::address_type_t::Page, 0x00),
        ddm::set_address_event_t(ddm::address_type_t::Block, 0x00),
        ddm::set_address_event_t(ddm::address_type_t::Byte, 0x00), ddm::do_transfer_event_t(), ddm::do_read_event_t(),
        ddm::do_transfer_event_t(), ddm::end_transfer_event_t() };

    for (auto& e : events)
    {
        EXPECT_TRUE(std::visit([&](auto&& arg) { return sm.process_event(arg); }, e));
    }
    EXPECT_TRUE(sm.is("X"_s));
}

void
action_impl(const ddm::initial_read_event_t& e) noexcept
{
    printf("action: %s\n", e.to_string().c_str());
}

TEST_F(BoostSMLUnitTest, overloaded_is_not_possible_transition)
{
    /* InitialRead -> SetAddr -> Transfer -> DoRead -> Transfer -> X */

    struct guard
    {
        bool operator()(const device_context_t& ctx, const ddm::initial_read_event_t& e) const noexcept
        {
            printf("guard: %s\n", e.to_string().c_str());
            return true;
        }
    };

    struct DeviceTransition
    {
        auto operator()() const
        {
            using namespace sml;
            return make_transition_table(
                *"idle"_s + event<ddm::initial_read_event_t>[guard {}] / wrap(action_impl) = "wait_for_addr"_s,
                "wait_for_addr"_s + event<ddm::set_address_event_t>                        = "wait_for_addr"_s,
                "wait_for_addr"_s + event<ddm::do_transfer_event_t>                        = "wait_for_action"_s,
                "wait_for_action"_s + event<ddm::do_read_event_t>                          = "transfer"_s,
                "transfer"_s + event<ddm::do_transfer_event_t>                             = "transfer"_s,
                "transfer"_s + event<ddm::end_transfer_event_t>                            = X);
        }
    };

    device_context_t          ctx {};
    sml::sm<DeviceTransition> sm { ctx };
    sm.process_event(ddm::initial_read_event_t(nullptr));

    std::vector<ddm::l2_event_t> events { ddm::initial_read_event_t(nullptr),
        ddm::set_address_event_t(ddm::address_type_t::Page, 0x00),
        ddm::set_address_event_t(ddm::address_type_t::Block, 0x00),
        ddm::set_address_event_t(ddm::address_type_t::Byte, 0x00), ddm::do_transfer_event_t(), ddm::do_read_event_t(),
        ddm::do_transfer_event_t(), ddm::end_transfer_event_t() };
}

struct runtime_event_t
{
    int id = 0;
};

struct event1_t
{
    static constexpr int id = 1;
};

TEST_F(BoostSMLUnitTest, dynamic_dispatch)
{
    device_context_t ctx {};

    struct dispatch_table
    {
        auto operator()() noexcept
        {
            using namespace sml;
            return make_transition_table(
                * "idle"_s + event<ddm::do_read_event_t> = "wait_for_addr"_s
                );
//               *"idle"_s + event<ddm::initial_read_event_t>         = "wait_for_addr"_s,
//                "wait_for_addr"_s + event<ddm::set_address_event_t> = "wait_for_addr"_s,
//                "wait_for_addr"_s + event<ddm::do_transfer_event_t> = "wait_for_action"_s,
//                "wait_for_action"_s + event<ddm::do_read_event_t>   = "transfer"_s,
//                "transfer"_s + event<ddm::do_transfer_event_t>      = "transfer"_s,
//                "transfer"_s + event<ddm::end_transfer_event_t>     = X);
        }
    };

    sml::sm<dispatch_table> sm { ctx };
    constexpr int l2_event_begin = (int) ddm::DEVICE_EVENT_TYPE_BEGIN + 1;
    constexpr int l2_event_end   = (int) ddm::MAX_DEVICE_EVENT_TYPES - 1;
    sm.process_event(ddm::initial_read_event_t(nullptr));
    auto dispatch = sml::utility::make_dispatch_table<runtime_event_t, l2_event_begin, l2_event_end>(sm);
//    auto dispatch_event = sml::utility::make_dispatch_table<
//        ddm::event_t, l2_event_begin, l2_event_end>(sm);

}
