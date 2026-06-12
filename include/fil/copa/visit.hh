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

#ifndef FIL_VISIT_HH
#define FIL_VISIT_HH

#include <memory>

#include "sink.hh"

namespace fil::copa {

// Helper trait to extract the return type from a callback
template<typename Callback, typename Node>
using callback_astnode_result_t = std::invoke_result_t<Callback, std::vector<int>, Node>;

template<typename Callback, typename T>
auto visit(Callback&& callback, const T& node) -> callback_astnode_result_t<Callback, T> {
    using result_type = callback_astnode_result_t<Callback, T>;
    return std::forward<Callback>(callback)(std::vector<result_type> {}, node);
}

template<typename Callback, ast_node_concept T>
auto visit(Callback&& callback, const T& node) -> callback_astnode_result_t<Callback, T> {
    return callback(
        std::array {
            visit(callback, node.lhs),
            visit(callback, node.rhs),
            visit(callback, node.value),
        },
        node);
}

template<typename Callback, typename T>
auto visit(Callback&& callback, const std::shared_ptr<T>& node) {
    return visit(std::forward<Callback>(callback), *node);
}

template<typename Callback, typename... Ts>
auto visit(Callback&& callback, const std::variant<Ts...>& node) {
    return std::visit([&callback](const auto& n) { return visit(std::forward<Callback>(callback), n); }, node);
}

} // namespace fil::copa

#endif // FIL_VISIT_HH
