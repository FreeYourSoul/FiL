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

    template<callback_type Cb, typename Value>
    constexpr void operator()(void* ctx, Cb cb, Value&& value) {
        cb(ctx, value_, std::forward<Value>(value));
    }
    template<member_type Mem, typename Value>
    constexpr void operator()(void* ctx, Mem mem, Value&& value) {
        mem(ctx, value_, std::forward<Value>(value));
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

template<typename ast_node, std::uint32_t Precedence>
class ast_tree_generator {
  public:
    struct ctx_extension {
        std::shared_ptr<ast_node> previous_node;
        std::uint32_t previous_precedence;
    };

    constexpr ast_tree_generator() = default;

    template<member_type Mem, typename Value>
    constexpr void operator()(ctx_extension* ctx, Mem mem, Value&& value) //
        = delete ("Bad usage of ast_tree_generator (cannot use fil::copa::member object");

    template<typename Value>
    constexpr void operator()(ctx_extension* ctx, ast_node::leaf, Value&& value) {
        if (node_ == nullptr) {
            node_ = std::make_shared<ast_node>();
        }
        node_->lhs_ = std::forward<Value>(value);
    }

    template<typename Value>
    constexpr void operator()(ctx_extension* ctx, ast_node::operand cb, Value&& value) {
        if (node_ == nullptr) {
            return;
        }

        node_->value = cb(std::forward<Value>(value));

        if (ctx->previous_node == nullptr /*first pass*/) {
            ctx->previous_node = node_;
        } else if (Precedence >= ctx->previous_precedence) {
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

    std::string value_;
    std::string operand_;
};

} // namespace fil::copa::sink

namespace fil::copa {

template<std::invocable<std::string> auto Callback>
struct ast_node {
    using operand_type = std::invoke_result_t<decltype(Callback), std::string>;

    operand_type value;
    std::variant<std::shared_ptr<ast_node>, std::string> lhs;
    std::variant<std::shared_ptr<ast_node>, std::string> rhs;

    struct operand : callback<Callback> {};
    struct leaf : callback<> {};
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
