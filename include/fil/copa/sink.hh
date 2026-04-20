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
    using ctx_extension = int;

    constexpr aggregator() = default;

    template<callback_type Cb, typename Value>
    constexpr void operator()(void*, Cb cb, Value&& value) {
        cb(std::forward<Value>(value));
    }
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
    using ctx_extension = int;

    constexpr void operator()(void*, const auto&, auto&&) {}
    constexpr value_type value() const { return {}; }
};

template<typename ast_node, std::uint32_t Precedence>
class ast_tree_generator {
  public:
    using value_type = ast_node;

    struct ctx_extension {
        std::shared_ptr<ast_node> tmp_node; //!< created each time a leaf is encountered

        std::shared_ptr<ast_node> current_node;
        std::shared_ptr<ast_node> previous_node;
        std::uint32_t previous_precedence;
    };

    constexpr ast_tree_generator() = default;

    template<member_type Mem, typename Value>
    constexpr void operator()(ctx_extension* ctx, Mem mem, Value&& value) //
        = delete ("Bad usage of ast_tree_generator (cannot use fil::copa::member object");

    template<typename Value>
    constexpr void operator()(ctx_extension*, member_noop, Value&&) {}

    template<typename Value>
    constexpr void operator()(ctx_extension* ctx, ast_node::leaf, Value&& value) {
        ctx->tmp_node      = std::make_shared<ast_node>();
        ctx->tmp_node->lhs = std::forward<Value>(value);

        std::println("leaf -- {} : {}", value, typeid(Value).name());

        if (ctx->current_node == nullptr) {
            ctx->current_node = ctx->tmp_node;
        }
        node_ = ctx->current_node.get();
    }

    template<typename Value>
    constexpr void operator()(ctx_extension* ctx, ast_node::operand cb, Value&& value) {
        static_assert(!std::is_void_v<std::invoke_result_t<typename ast_node::operand, Value>>);

        ctx->current_node->value = cb(std::forward<Value>(value));

        std::println("operand -- {} : {}", value, typeid(Value).name());

        if (ctx->previous_node == nullptr /*first pass*/) {
            ctx->previous_node = std::move(ctx->current_node);
        } else if (Precedence >= ctx->previous_precedence) {
            ctx->previous_node->rhs = std::move(ctx->current_node);
            ctx->current_node       = ctx->previous_node;
        } else {
            ctx->previous_node->rhs = std::forward<Value>(value);
            ctx->current_node->lhs  = std::move(ctx->previous_node);
            ctx->previous_node      = ctx->current_node;
        }

        ctx->previous_precedence = Precedence;

        node_ = ctx->current_node.get();
    }

    constexpr value_type value() const {
        if (!node_)
            return {};
        return *node_;
    }

  private:
    ast_node* node_ = nullptr;

    std::string value_;
    std::string operand_;
};

} // namespace fil::copa::sink

namespace fil::copa {

template<std::invocable<std::string> auto Callback>
struct ast_node {
    using operand_type = std::invoke_result_t<decltype(Callback), std::string>;

    operand_type value;
    std::variant<std::shared_ptr<ast_node>, std::string, int, char> lhs;
    std::variant<std::shared_ptr<ast_node>, std::string, int, char> rhs;

    struct operand : callback<Callback> {};
    struct leaf : callback<[](const std::string& value) { return value; }> {};
};

} // namespace fil::copa

template<typename T>
struct fil::shallow_copy<fil::copa::sink::aggregator<T>> {
    static constexpr auto copy(copa::sink::aggregator<T>& object) {
        copa::sink::aggregator<T&> shallow {object.value_};
        return shallow;
    }
}; // namespace fil

#endif // FIL_SINK_HH
