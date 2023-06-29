#include <boost/sml.hpp>
#include <context.h>
#include <cstdio>
#include <events.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include <utility>
#include <vector>

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

template<ddm::Event E>
struct guard
{
    bool operator()(device_context_t & ctx, const E &e) const
    {
        printf("guard: %s\n", e.to_string().c_str());
        return true;
    }
};


template<ddm::Event E>
struct action
{
    void operator()(device_context_t & ctx, const E &e) const
    {
        printf("action: %s\n", e.to_string().c_str());
    }
};

#define TRANS(FROM, EVENT, TO) \
    FROM##_s + event<ddm::EVENT>[guard<ddm::EVENT>{}] / action<ddm::EVENT>{} = TO##_s

template<> auto sml::state<class X_s> = sml::X;

TEST_F(BoostSMLUnitTest, make_transition)
{
    /* InitialRead -> SetAddr -> Transfer -> DoRead -> Transfer -> X */
    device_context_t ctx {};

    struct DeviceTransition
    {
        auto operator()() const
        {
            using namespace sml;
            return make_transition_table(
                *TRANS("idle", initial_read_event_t, "wait_for_addr"),
                TRANS("wait_for_addr", set_address_event_t, "wait_for_addr"),
                TRANS("wait_for_addr", do_transfer_event_t, "wait_for_action"),
                TRANS("wait_for_action", do_read_event_t, "transfer"),
                TRANS("transfer", do_transfer_event_t, "transfer"),
                TRANS("transfer", end_transfer_event_t, "X")
            );
        }
    };

    DeviceTransition          trans {};
    sml::sm<DeviceTransition> sm { trans, ctx };

    using namespace sml;

    uint8_t * buf = nullptr;
    EXPECT_TRUE(sm.is("idle"_s));

    sm.process_event(ddm::initial_read_event_t(buf));
    EXPECT_TRUE(sm.is("wait_for_addr"_s));

    sm.process_event(ddm::set_address_event_t(ddm::address_type_t::Page, 0x00));
    EXPECT_TRUE(sm.is("wait_for_addr"_s));

    sm.process_event(ddm::set_address_event_t(ddm::address_type_t::Block, 0x00));
    EXPECT_TRUE(sm.is("wait_for_addr"_s));

    sm.process_event(ddm::set_address_event_t(ddm::address_type_t::Byte, 0x00));
    EXPECT_TRUE(sm.is("wait_for_addr"_s));

    sm.process_event(ddm::do_transfer_event_t());
    EXPECT_TRUE(sm.is("wait_for_action"_s));

    sm.process_event(ddm::do_read_event_t());
    EXPECT_TRUE(sm.is("transfer"_s));

    sm.process_event(ddm::do_transfer_event_t());
    EXPECT_TRUE(sm.is("transfer"_s));

    sm.process_event(ddm::end_transfer_event_t());
    EXPECT_TRUE(sm.is("X"_s));
}

struct guard_overload
{
    bool operator()(device_context_t & ctx, const ddm::initial_read_event_t &e) const
    {
        printf("guard: %s\n", e.to_string().c_str());
        return true;
    }
    bool operator()(device_context_t & ctx, const ddm::set_address_event_t &e) const
    {
        printf("guard: %s\n", e.to_string().c_str());
        return true;
    }    bool operator()(device_context_t & ctx, const ddm::do_transfer_event_t &e) const
    {
        printf("guard: %s\n", e.to_string().c_str());
        return true;
    }    bool operator()(device_context_t & ctx, const ddm::do_read_event_t &e) const
    {
        printf("guard: %s\n", e.to_string().c_str());
        return true;
    }    bool operator()(device_context_t & ctx, const ddm::end_transfer_event_t &e) const
    {
        printf("guard: %s\n", e.to_string().c_str());
        return true;
    }
};

struct action_overload
{
    void operator()(device_context_t & ctx, const ddm::initial_read_event_t &e) const
    {
        printf("action: %s\n", e.to_string().c_str());
    }
    void operator()(device_context_t & ctx, const ddm::set_address_event_t &e) const
    {
        printf("action: %s\n", e.to_string().c_str());
    }
    void operator()(device_context_t & ctx, const ddm::do_transfer_event_t &e) const
    {
        printf("action: %s\n", e.to_string().c_str());
    }
    void operator()(device_context_t & ctx, const ddm::do_read_event_t &e) const
    {
        printf("action: %s\n", e.to_string().c_str());
    }
    void operator()(device_context_t & ctx, const ddm::end_transfer_event_t &e) const
    {
        printf("action: %s\n", e.to_string().c_str());
    }
};

TEST_F(BoostSMLUnitTest, overloaded_transition)
{

}
