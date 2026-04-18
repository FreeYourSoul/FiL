#ifndef FIL_SINK_HH
#define FIL_SINK_HH

#include <memory>
#include <variant>

#include "fil/copa/member.hh"
#include "fil/meta/shallow_copy.hh"

namespace fil::copa::sink {
template<typename T>
class aggregator {

    template<typename>
    friend struct fil::shallow_copy;

  public:
    using value_type    = T;
    using ctx_extension = void;

    constexpr aggregator() = default;

    template<member_type Mem, typename Value>
    constexpr void operator()(void*, Mem mem, Value&& value) {
        mem(value_, std::forward<Value>(value));
    }

    constexpr const value_type& value() const { return value_; }

  private:
    value_type value_ {};
};

template<typename T = char>
struct convertor_noop {
    using value_type    = T;
    using ctx_extension = void;

    constexpr void operator()(void*, const auto&, auto&&) {}
    constexpr value_type value() const { return {}; }
};

template<typename ast_node, std::uint32_t Precedence, std::invocable<std::string&&> auto generate_node_func>
class ast_tree_generator {
  public:
    struct ctx_extension {
        std::shared_ptr<ast_node> previous_node;
        std::uint32_t previous_precedence;
    };

    constexpr ast_tree_generator() = default;

    constexpr void operator()(ctx_extension* ctx, std::variant<ast_node, std::string>&& value) {
        if (node_ == nullptr) {
            op_value = std::move(value);
            node_    = std::make_shared<ast_node>();
            return;
        }

        const bool first_pass = ctx->previous_node == nullptr;

        node_->value_ = generate_node_func(std::move(op_value));
        if (Precedence >= ctx->previous_precedence || first_pass) {
            node_->lhs_              = std::move(value);
            ctx->previous_node->rhs_ = std::move(node_);
            node_                    = ctx->previous_node;
        } else {
            ctx->previous_node->rhs_ = std::move(value);
            node_->lhs_              = std::move(ctx->previous_node);
            ctx->previous_node       = node_;
        }

        ctx->previous_precedence = Precedence;
    }

    constexpr const ast_node& value() const { return node_; }

  private:
    std::shared_ptr<ast_node> node_ = nullptr;

    std::string op_value;
};

} // namespace fil::copa::sink

namespace fil::copa {
template<typename T>
struct ast_node {
    T value;
    std::variant<std::shared_ptr<ast_node>, std::string> lhs;
    std::variant<std::shared_ptr<ast_node>, std::string> rhs;
};
} // namespace fil::copa

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
