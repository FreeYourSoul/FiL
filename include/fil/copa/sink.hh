#ifndef FIL_SINK_HH
#define FIL_SINK_HH

#include "debug.hh"

#include <memory>
#include <variant>

#include "fil/copa/member.hh"
#include "fil/meta/shallow_copy.hh"

/**
 * @brief A convertor that aggregates parsed values into a single AST object.
 *
 * @details The `aggregator` is a generic converter used in Copa to directly assign or process
 * parsed values and populate an `ast_object`. Unlike `ast_tree_generator`, which builds complex
 * hierarchical trees with operator precedence handling, `aggregator` provides a simple, direct
 * mapping from matched rules to struct members or callbacks.
 *
 * ## When to Use
 *
 * Use `aggregator` when:
 * - Your grammar directly maps to a simple data structure (not a tree)
 * - You want straightforward member assignment without tree construction
 * - You don't need operator precedence or hierarchical AST building
 *
 * @tparam T The AST object type that will hold the parsing results.
 *           - Must be default-constructible
 *           - Must support move assignment and copy assignment
 *           - Typically a struct with fields matching your grammar rules
 *
 * @see fil::copa::member
 * @see fil::copa::callback
 * @see fil::copa::production
 */
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

/**
 * @brief A convertor that builds binary expression trees with operator precedence handling.
 *
 * @details `ast_tree_generator` constructs hierarchical binary trees suitable for parsing expressions
 * with operators of varying precedence levels. It automatically restructures the tree during parsing
 * to ensure the correct precedence binding without post-processing.
 *
 * ## When to Use
 *
 * Use `ast_tree_generator` when:
 * - Parsing mathematical expressions or similar operator-based grammars
 * - Operators have different precedence levels (e.g., `*` binds tighter than `+`)
 * - You need a tree where higher-precedence operators appear deeper in the tree
 * - Parsing recursive expression grammars with multiple precedence levels
 *
 * ## Template Parameters
 *
 * @tparam ast_node The AST node type. Must be a @c fil::copa::ast_node<CallbackOp> where:
 *   - `CallbackOp` is a callable that transforms operator tokens to meaningful values
 *   - The node has `value`, `lhs`, and `rhs` members
 *   - Leaf and operand callbacks are defined as nested types
 *
 * @see fil::copa::ast_node
 * @see fil::copa::sink::aggregator
 * @see fil::copa::production
 */
template<typename ast_node>
class ast_tree_generator {
  public:
    using value_type = ast_node;

    struct ctx_extension {
        std::shared_ptr<ast_node> tmp_node; //!< created each time a leaf is encountered

        std::shared_ptr<ast_node> current_node;
        std::shared_ptr<ast_node> previous_node;
        std::uint32_t previous_precedence {0};
    };

    explicit constexpr ast_tree_generator(std::uint32_t precedence = 0)
        : precedence_(precedence) {}

    template<member_type Mem, typename Value>
    constexpr void operator()(ctx_extension* ctx, Mem mem, Value&& value) //
        = delete ("Bad usage of ast_tree_generator (cannot use fil::copa::member object");

    template<typename Value>
    constexpr void operator()(ctx_extension*, member_noop, Value&&) {}

    template<typename Value>
    constexpr void operator()(ctx_extension* ctx, ast_node::leaf, Value&& value) {
        ctx->tmp_node = std::make_shared<ast_node>();
        if constexpr (std::is_same_v<ast_node, Value>) {
            auto node_ptr      = std::make_shared<Value>();
            *node_ptr          = std::forward<Value>(value);
            ctx->tmp_node->lhs = std::move(node_ptr);
        } else {
            ctx->tmp_node->lhs = std::forward<Value>(value);
        }
    }

    template<typename Value>
    constexpr void operator()(ctx_extension* ctx, ast_node::operand cb, Value&& value) {
        static_assert(!std::is_void_v<std::invoke_result_t<typename ast_node::operand, Value>>, //
                      "An operand cannot have a callback returning void : this callback must be used to set the value of the ast_node.");

        if (ctx->tmp_node == nullptr) {
            return;
        }

        ctx->tmp_node->value = cb(std::forward<Value>(value));

        if (ctx->previous_node == nullptr /*first pass*/) {
            ctx->previous_node = std::move(ctx->tmp_node);
            ctx->current_node  = ctx->previous_node;
        } else if (precedence_ >= ctx->previous_precedence) {
            ctx->previous_node->rhs = std::move(ctx->tmp_node);
            ctx->previous_node      = std::get<std::shared_ptr<ast_node>>(ctx->previous_node->rhs);
        } else {
            ctx->previous_node->rhs = std::move(ctx->tmp_node->lhs);
            ctx->tmp_node->lhs      = ctx->current_node;
            ctx->current_node       = std::move(ctx->tmp_node);
            ctx->previous_node      = ctx->current_node;
        }

        ctx->previous_precedence = precedence_;
    }

    constexpr value_type value(ctx_extension* ctx) const {
        ast_node value_node;
        if (ctx->current_node) {
            value_node.value = ctx->current_node->value;
            value_node.lhs   = ctx->current_node->lhs;
            value_node.rhs   = ctx->current_node->rhs;

            if (ctx->previous_node && ctx->tmp_node) {
                ctx->previous_node->rhs = ctx->tmp_node->lhs;
            }
        }

        if (ctx->tmp_node && (!value_node.lhs.index() && !value_node.rhs.index())) {
            value_node = *ctx->tmp_node;
        }
        return value_node;
    }

  private:
    std::uint32_t precedence_; //!< precedence of the current instance of the generator, tree construction depends on that difference
};

} // namespace fil::copa::sink

namespace fil::copa {

template<std::invocable<std::string> auto CallbackOp>
struct ast_node {
    using operand_type = std::invoke_result_t<decltype(CallbackOp), std::string>;

    operand_type value;
    std::variant<std::monostate, std::shared_ptr<ast_node>, std::string, int, char> lhs;
    std::variant<std::monostate, std::shared_ptr<ast_node>, std::string, int, char> rhs;

    struct operand : callback<CallbackOp> {};
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
