/// MIT License
//
// Copyright (c) 2025 Quentin Balland
// Repository : https://github.com/FreeYourSoul/FiL
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//         of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
//         to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//         copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
//         copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//         AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#ifndef FIL_SINK_HH
#define FIL_SINK_HH

#include <memory>
#include <variant>

#include "fil/copa/debug.hh"
#include "fil/copa/debug_details.hh"
#include "fil/copa/member.hh"
#include "fil/meta/shallow_copy.hh"
#include "fil/meta/typename.hh"

namespace fil::copa::sink {

namespace debug_aggregation {

template<typename T>
concept with_debug_info_type = requires(const T& t) {
    { t.copa_debug_info } -> std::convertible_to<debug_info>;
};

//! no-op version in case an aggregate object doesn't contain required @c debug_info member
void aggregate_debug_info(const auto&, auto&) { /*no-op if no debug info defined*/ }

/**
 * @brief aggregate the debugging information if the structure contains a @c copa_debug_info member of @c debug_info type
 *
 * @param ctx parsing context
 * @param aggregate object to aggregate
 */
void aggregate_debug_info(const auto& ctx, with_debug_info_type auto& aggregate) {
    aggregate.copa_debug_info = debug_info {
        .token  = ctx.current_token,
        .line   = ctx.current_line,
        .cursor = ctx.reader->reader_cursor(),
    };
}

} // namespace debug_aggregation

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
        using cb_result_t = std::invoke_result_t<Cb, Value>;
        if constexpr (std::is_void_v<cb_result_t>) {
            // Callback returns nothing => only side effects, no assignment to value_
            cb(std::forward<Value>(value));
        } else {
            // Callback returns a value => it must be convertible to value_type (T)
            static_assert(std::is_convertible_v<cb_result_t, value_type>,
                          "aggregator: callback return type must be convertible to the aggregated value_type (T).");
            value_ = cb(std::forward<Value>(value));
        }
    }
    template<member_type Mem, typename Value>
    constexpr void operator()(void*, Mem mem, Value&& value) {
        mem(value_, std::forward<Value>(value));
    }

    constexpr const value_type& value(auto& ctx) {
        debug_aggregation::aggregate_debug_info(ctx, value_);
        return value_;
    }

  private:
    value_type value_ {};
};

template<typename T = char>
struct convertor_noop {
    using value_type    = T;
    using ctx_extension = int;

    constexpr void operator()(void*, const auto&, auto&&) {}
    constexpr value_type value(auto&) const { return {}; }
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

    constexpr value_type value(auto& ctx) {
        ctx_extension* ctx_ext = ctx.convertor_ctx;

        ast_node value_node;
        if (ctx_ext->current_node) {
            value_node.value = ctx_ext->current_node->value;
            value_node.lhs   = ctx_ext->current_node->lhs;
            value_node.rhs   = ctx_ext->current_node->rhs;

            if (ctx_ext->previous_node && ctx_ext->tmp_node) {
                ctx_ext->previous_node->rhs = ctx_ext->tmp_node->lhs;
            }
        }
        if (ctx_ext->tmp_node && (!value_node.lhs.index() && !value_node.rhs.index())) {
            value_node = *ctx_ext->tmp_node;
        }

        debug_aggregation::aggregate_debug_info(ctx, value_node);

        return value_node;
    }

  private:
    std::uint32_t precedence_; //!< precedence of the current instance of the generator, tree construction depends on that difference
};

} // namespace fil::copa::sink

namespace fil::copa {

template<std::invocable<std::string> auto CallbackOp, typename... Ts>
struct ast_node {
    struct is_copa_ast_node; // flag for concept

    using operand_type = std::invoke_result_t<decltype(CallbackOp), std::string>;
    using node_type    = std::variant<std::monostate, std::shared_ptr<ast_node>, std::string, int, char, Ts...>;

    operand_type value;
    node_type lhs = std::monostate {};
    node_type rhs = std::monostate {};

    struct operand : callback<CallbackOp> {};
    struct leaf : callback<[](const std::string& value) { return value; }> {};

    [[nodiscard]] std::string to_string() const { return debug_details_::ast_tree_to_string(*this); }
};

template<typename T>
concept ast_node_concept = requires(T& t) {
    typename T::is_copa_ast_node;
    typename T::operand_type;
    typename T::node_type;
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
