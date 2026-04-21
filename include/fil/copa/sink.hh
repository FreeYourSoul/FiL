#ifndef FIL_SINK_HH
#define FIL_SINK_HH

#include <memory>
#include <variant>

#include "fil/copa/member.hh"
#include "fil/meta/shallow_copy.hh"

template<typename T>
void print_operand_tree(const T& operand, int depth, bool is_last);

template<typename T>
void print_ast_tree(const T& node, int depth = 0, bool is_right = false, bool is_last = true) {
    // Print tree branch characters
    std::string indent(depth * 3, ' ');
    std::print("{}", indent);

    if (depth > 0) {
        std::print("{}", (is_last ? "└── " : "├── "));
    }

    std::println("Value: {}", int(node.value));

    // Count non-empty children
    int child_count = 0;
    if (!node.lhs.valueless_by_exception())
        child_count++;
    if (!node.rhs.valueless_by_exception())
        child_count++;
    // Print left child
    if (!node.lhs.valueless_by_exception()) {
        bool is_last_child = (child_count == 1) || !node.rhs.valueless_by_exception();
        std::println("{}{} L:", indent, (is_last_child ? "    " : "│   "));
        std::visit([depth, is_last_child](auto& operand) { print_operand_tree(operand, depth + 2, is_last_child); }, node.lhs);
    }

    // Print right child
    if (!node.rhs.valueless_by_exception()) {
        std::println("{}    R:", indent);
        std::visit([depth](auto& operand) { print_operand_tree(operand, depth + 2, true); }, node.rhs);
    }
}

template<typename T>
void print_operand_tree(const T& operand, int depth, bool is_last) {
    std::string indent(depth * 3, ' ');
    std::print("{}{}", indent, (is_last ? "└── " : "├── "));

    if constexpr (std::is_same_v<T, std::string>) {
        std::println("String: \"{}\"", operand);
    } else if constexpr (std::is_same_v<T, int>) {
        std::println("Int: \"{}\"", operand);
    } else if constexpr (std::is_same_v<T, char>) {
        std::println("Char: \"{}\"", operand);
    } else {
        if (operand) {
            std::println("");
            print_ast_tree(*operand, depth, false, is_last);
        } else {
            std::println("nullptr");
        }
    }
}

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

    constexpr const value_type& value(auto*) const { return value_; }

  private:
    value_type value_ {};
};

template<typename T = char>
struct convertor_noop {
    using value_type    = T;
    using ctx_extension = int;

    constexpr void operator()(void*, const auto&, auto&&) {}
    constexpr value_type value(auto*) const { return {}; }
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

        std::size_t count_in {0}; //!< testing @todo : remove
    };

    constexpr ast_tree_generator() = default;

    template<member_type Mem, typename Value>
    constexpr void operator()(ctx_extension* ctx, Mem mem, Value&& value) //
        = delete ("Bad usage of ast_tree_generator (cannot use fil::copa::member object");

    template<typename Value>
    constexpr void operator()(ctx_extension*, member_noop, Value&&) {}

    template<typename Value>
    constexpr void operator()(ctx_extension* ctx, ast_node::leaf, Value&& value) {
        get_node_ = [ctx]() { return ctx->current_node; };

        ++ctx->count_in;

        ctx->tmp_node      = std::make_shared<ast_node>();
        ctx->tmp_node->lhs = std::forward<Value>(value);

        std::println("leaf -- {} : {} ", value, typeid(Value).name(), ctx->count_in);
    }

    template<typename Value>
    constexpr void operator()(ctx_extension* ctx, ast_node::operand cb, Value&& value) {

        if (ctx->tmp_node == nullptr) {
            std::println("tmp node is null -- error");
            return;
        }

        get_node_ = [ctx]() { return ctx->current_node; };
        ++ctx->count_in;

        static_assert(!std::is_void_v<std::invoke_result_t<typename ast_node::operand, Value>>);

        ctx->tmp_node->value = cb(std::forward<Value>(value));

        if (ctx->previous_node == nullptr /*first pass*/) {
            ctx->previous_node = std::move(ctx->tmp_node);
            std::println("first-pass -- {} ", ctx->count_in);
        } else if (Precedence >= ctx->previous_precedence) {
            ctx->previous_node->rhs = std::move(ctx->tmp_node);
            ctx->current_node       = ctx->previous_node;
            std::println("precedence-higher -- {} ", ctx->count_in);
        } else {
            ctx->previous_node->rhs = std::forward<Value>(value);
            ctx->tmp_node->lhs      = std::move(ctx->previous_node);
            ctx->previous_node      = ctx->tmp_node;
            std::println("precedence-lower -- {} ", ctx->count_in);
        }

        ctx->previous_precedence = Precedence;
    }

    constexpr value_type value(ctx_extension* ctx) const {
        ast_node value_node;
        if (!ctx->current_node) {
            if (!ctx->previous_node) {
                return {};
            }
            value_node = *ctx->previous_node;
            if (ctx->tmp_node) {
                value_node.rhs = ctx->tmp_node->lhs;
            }
        }
        print_ast_tree(value_node);
        return value_node;
    }

  private:
    std::function<std::shared_ptr<ast_node>()> get_node_;
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
