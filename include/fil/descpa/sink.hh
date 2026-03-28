#ifndef FIL_SINK_HH
#define FIL_SINK_HH

#include "fil/descpa/descpa.hh"

namespace fil::descpa::sink {

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
    constexpr void operator()(const auto&, auto&&) {}
    constexpr int value() const { return 0; }
};

} // namespace fil::descpa::sink

#endif // FIL_SINK_HH
