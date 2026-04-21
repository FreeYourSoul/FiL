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

namespace fil::copa::debug {

template<typename T>
void print_operand_tree(const T& operand, int depth, bool is_last);

template<typename T>
void print_ast_tree(const T& node, int depth = 0, bool is_right = false, bool is_last = true) {
    // Print tree branch characters
    std::string indent(depth * 3, '.');
    std::println("{}Value: {}", indent, int(node.value));

    // Count non-empty children
    int child_count = 0;
    if (!node.lhs.valueless_by_exception())
        child_count++;
    if (!node.rhs.valueless_by_exception())
        child_count++;
    // Print left child
    if (!node.lhs.valueless_by_exception()) {
        bool is_last_child = (child_count == 1) || !node.rhs.valueless_by_exception();
        std::println("{}..{} L:", indent, (is_last_child ? "...." : "│   "));
        std::visit([depth, is_last_child](auto& operand) { print_operand_tree(operand, depth + 2, is_last_child); }, node.lhs);
    }

    // Print right child
    if (!node.rhs.valueless_by_exception()) {
        std::println("..{}.... R:", indent);
        std::visit([depth](auto& operand) { print_operand_tree(operand, depth + 2, true); }, node.rhs);
    }
}

template<typename T>
void print_operand_tree(const T& operand, int depth, bool is_last) {
    std::string indent(depth * 3, '.');
    std::print("{}  {}", indent, (is_last ? "└── " : "├── "));

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

} // namespace fil::copa::debug

#endif // FIL_DEBUG_HH
