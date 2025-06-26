// MIT License
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

#pragma once

#include <map>

namespace fil {

template<typename T = int> class boundary_map {

    using iterator       = typename std::map<int, T>::iterator;
    using const_iterator = typename std::map<int, T>::const_iterator;

  public:
    iterator begin() { return _map.begin(); }
    const_iterator begin() const { return _map.begin(); }

    iterator end() { return _map.end(); }
    const_iterator end() const { return _map.end(); }

    [[nodiscard]] auto size() const { return _map.size(); }
    [[nodiscard]] bool empty() const { return _map.empty(); }

    [[nodiscard]] auto get(int index) const { return _map.lower_bound(index); }

    void insert(int index, T&& element) {
        auto it = get(index);
        if (it == _map.end()) {
            _map[index] = std::forward<T>(element);
        } else if (element != it->second) {
            _map[it->first + 1] = std::forward<T>(element);
        } else {
            _map.erase(it);
            _map[it->first] = std::forward<T>(element);
        }
    }

    void insert(int index, const T& element) {
        T elem = element;
        insert(index, std::move(elem));
    }

  private:
    std::map<int, T> _map;
};

using boundary_map_int = boundary_map<int>;

} // namespace fil