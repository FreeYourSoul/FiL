#ifndef FIL_SINK_HH
#define FIL_SINK_HH

#include "fil/copa/member.hh"

namespace fil::copa::sink {

template<typename T>
class aggregator {
  public:
    using value_type = T;

    constexpr aggregator() = default;

    template<member_type Mem, typename Value>
    constexpr void operator()(Mem mem, Value&& value) {
        mem(value_, std::forward<Value>(value));
    }

    constexpr const value_type& value() const { return value_; }

  private:
    value_type value_ {};
};

struct convertor_noop {
    using value_type = int;

    constexpr void operator()(const auto&, auto&&) {}
    constexpr int value() const { return 0; }
};

} // namespace fil::copa::sink

#endif // FIL_SINK_HH
