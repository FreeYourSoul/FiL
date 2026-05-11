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

#ifndef FIL_DEBUG_HH
#define FIL_DEBUG_HH

#include <print>
#include <string>
#include <variant>

#include "fil/algorithm/string.hh"
#include "fil/meta/type_traits.hpp"

namespace fil::copa::debug {

template<typename T>
std::string ast_node_to_string(const T& operand, int depth, bool is_last);

template<typename T>
std::string ast_tree_to_string(const T& node, int depth = 0, bool is_last = true) {
    std::string result;

    // Print tree branch characters
    std::string indent(depth * 3, '.');
    result += std::format("{}Value: {}\n", indent, fil::to_string(node.value));

    // Count non-empty children
    int child_count = 0;
    if (!node.lhs.valueless_by_exception())
        child_count++;
    if (!node.rhs.valueless_by_exception())
        child_count++;
    // Print left child
    if (!node.lhs.valueless_by_exception()) {
        bool is_last_child = (child_count == 1) || !node.rhs.valueless_by_exception();
        result += std::format("{}..{} L:\n", indent, (is_last_child ? "...." : "│   "));
        result += //
            std::visit([depth, is_last_child](auto& operand) { return ast_node_to_string(operand, depth + 2, is_last_child); }, node.lhs);
    }

    // Print right child
    if (!node.rhs.valueless_by_exception()) {
        result += std::format("..{}.... R:\n", indent);
        result += //
            std::visit([depth](auto& operand) { return ast_node_to_string(operand, depth + 2, true); }, node.rhs);
    }
    return result;
}

template<typename T>
std::string ast_tree_to_string(const std::shared_ptr<T>& node, int depth, bool is_right, bool is_last) {
    return ast_tree_to_string(*node, depth, is_right, is_last);
}

template<typename T>
std::string ast_node_to_string(const T& operand, int depth, bool is_last) {
    std::string result;

    std::string indent(depth * 3, '.');
    result += std::format("{}  {}", indent, (is_last ? "└── " : "├── "));

    if constexpr (std::is_same_v<T, std::monostate>) {
        result += "<noset>\n";
    } else if constexpr (std::is_same_v<T, std::string>) {
        result += std::format("String: \"{}\"\n", operand);
    } else if constexpr (std::is_same_v<T, int>) {
        result += std::format("Int: \"{}\"\n", operand);
    } else if constexpr (std::is_same_v<T, char>) {
        result += std::format("Char: \"{}\"\n", operand);
    } else if constexpr (is_stringifiable<T>) {
        result += fil::to_string(operand) + "\n";
    } else {
        if (operand) {
            result += std::format("\n{}", ast_tree_to_string(*operand, depth, is_last));
        } else {
            result += "nullptr\n";
        }
    }
    return result;
}
} // namespace fil::copa::debug

#endif // FIL_DEBUG_HH
