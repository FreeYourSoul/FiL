#ifndef FIL_SINK_HH
#define FIL_SINK_HH

#include <memory>

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

template<typename T>
concept tree_node_type = requires(T& t) {
    typename T::node;
    { t.right_hand_side };
    { t.left_hand_side };
};

template<tree_node_type T>
struct ast_node {
    using node = T::node;
    using lhs  = decltype(T::lhs);
    using rhs  = decltype(T::rhs);

    node node_;
    std::shared_ptr<lhs> lhs_;
    std::shared_ptr<rhs> rhs_;
};

template<tree_node_type T>
class ast_tree_generator {

  public:
    constexpr ast_tree_generator() = default;

    template<typename Value, typename Mem>
    constexpr void operator()(Mem mem, Value&& value) =
        delete ("wrong usage of ast_tree generator : this shall not be used with matcher taking member types");

    template<typename Value> // partial template specialization no-op
    constexpr void operator()(member_noop, Value&& value) { /*no-op*/ }

    template<typename Value> // partial template specialization lhs
    constexpr void operator()(ast_node_lhs, Value&& value) {}

    template<typename Value> // partial template specialization rhs
    constexpr void operator()(ast_node_rhs, Value&& value) {}

  private:
    ast_node<T> node_;
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
