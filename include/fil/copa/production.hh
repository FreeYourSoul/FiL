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

#ifndef FIL_PRODUCTION_HH
#define FIL_PRODUCTION_HH

#include "rule.hh"

namespace fil::copa {

template<typename T>
concept production = requires {
    typename T::ast_object;
    std::is_default_constructible_v<typename T::ast_object>;
    std::is_move_assignable_v<typename T::ast_object>;
    std::is_copy_assignable_v<typename T::ast_object>;

    { T::rules() } -> rule;
    { T::convertor() };
};

/**
 * @tparam Prod production with extra requirements to provide ignore() returning a rule of character to ignore
 * @return rule defined by the production Prod::ignore static member function
 */
template<production Prod>
requires requires {
    { Prod::ignore() } -> rule;
}
rule auto retrieve_ignore_rules(const Prod&) {
    return Prod::ignore();
}

} // namespace fil::copa

#endif // FIL_PRODUCTION_HH
