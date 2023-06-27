#include <boost/sml.hpp>
#include <cstdio>
#include <stdexcept>
#include <utility>

namespace sml = boost::sml;

struct e1 { };
struct e2 { };

auto guard = [] { return false; };
auto action = [] {};

using namespace sml;
class machine
{
public:
    auto operator()() const
    {
        using namespace sml;
        return make_transition_table(
            *"idle"_s + event<e1>[guard] / action = "s1"_s,
             "s1"_s + event<e2> / action = sml::X
            );
    }
};

template<typename M, typename E>
inline void at(M& m, E&& e)
{
    bool rv = m.process_event(std::forward<E>(e));
    if (!rv) {
        std::printf("state transition failed on event %s \n", typeid(E).name());
        throw std::runtime_error("state transition failed");
    }
}

int main()
{
    sml::sm<machine> sm;
    at(sm, e1{});
    at(sm, e2{});

    sm.visit_current_states([](auto state) { std::printf("current state: %s\n", state.c_str()); });

    return 0;
}
