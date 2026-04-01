#ifndef FIL_SINK_HH
#define FIL_SINK_HH

#include "fil/copa/member.hh"
#include "fil/meta/shallow_copy.hh"

namespace fil::copa::sink {

template<typename T>
class aggregator {

    template<typename>
    friend struct fil::shallow_copy;

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

template<typename T = char>
struct convertor_noop {
    using value_type = T;

    constexpr void operator()(const auto&, auto&&) {}
    constexpr value_type value() const { return {}; }
};

} // namespace fil::copa::sink

namespace fil {

namespace fil {}
template<typename T>
struct shallow_copy<copa::sink::aggregator<T>> {
    static constexpr auto copy(copa::sink::aggregator<T>& object) {
        copa::sink::aggregator<T&> shallow {object.value_};
        return shallow;
    }
};

} // namespace fil

#endif // FIL_SINK_HH
