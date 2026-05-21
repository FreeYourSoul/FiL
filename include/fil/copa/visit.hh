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

template<typename T>
concept visit_result = requires(T& t) { std::is_trivially_constructible_v<T>; };

template<typename Res, typename Callback, typename T>
Res visit(Callback&& callback, const T& node) {
    return std::forward<Callback>(callback)(std::vector<Res> {}, node);
}

template<typename Res, typename Callback, ast_node_concept T>
Res visit(Callback&& callback, const T& node) {
    return callback(
        std::array<Res, 2> {
            visit<Res>(callback, node.lhs),
            visit<Res>(callback, node.rhs),
        },
        node);
}

template<typename Res, typename Callback, typename T>
Res visit(Callback&& callback, const std::shared_ptr<T>& node) {
    return visit<Res>(std::forward<Callback>(callback), *node);
}

template<typename Res, typename Callback, typename... Ts>
Res visit(Callback&& callback, const std::variant<Ts...>& node) {
    return std::visit([&callback](const auto& n) { return visit<Res>(std::forward<Callback>(callback), n); }, node);
}

} // namespace fil::copa

#endif // FIL_VISIT_HH
