// MIT License
//
// Copyright (c) 2020 Quentin Balland
// Repository : https://github.com/FreeYourSoul/FyS
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

#ifndef FIL_RNG_HH
#define FIL_RNG_HH

#include <type_traits>
#include <memory>
#include <random>
#include <chrono>

#include <fmt/format.h>

namespace fil {

class rng {

public:
	explicit rng(std::uint32_t seed = 0)
			:_mt(std::make_unique<std::mt19937>()), _seed(seed)
	{
		if (!seed) {
			std::random_device rd;

			if (rd.entropy() != 0.0) {
				_seed = rd();
			}
			else {
				_seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
			}
		}
		_mt->seed(_seed);
	}

	template<typename Type>
	[[nodiscard]] Type generate_in_range(Type rA, Type rB)
	{
		static_assert(std::is_integral_v<Type> || std::is_floating_point_v<Type>,
				"Type has to be an integer or a floating point number");

		if constexpr (std::is_integral_v<Type>) {
			std::uniform_int_distribution<Type> distribution(rA, rB);
			return distribution(*_mt);
		}
		else if constexpr (std::is_floating_point_v<Type>) {
			std::uniform_real_distribution<Type> distribution(rA, rB);
			return distribution(*_mt);
		}
	}

	[[nodiscard]] std::uint32_t get_seed() const { return _seed; }

private:

	std::unique_ptr<std::mt19937> _mt;
	std::uint32_t _seed;
};

class uuid_generator {
public:
	explicit uuid_generator(const std::string& salt, std::uint32_t seed = 0)
			:_rng(seed)
	{
		for (char c : salt) {
			_salt += std::uint64_t(c);
		}
	}

	std::string generate_uuid(std::uint32_t max_size = 16u)
	{
		auto uuid = fmt::format("{:x}{:x}{:x}", _salt, _id, _rng.generate_in_range(10000, 1000000));
		if (uuid.size() > max_size) {
			uuid.resize(max_size);
		}
		return uuid;
	}

private:
	rng _rng;
	std::uint64_t _salt{};
	std::uint64_t _id{};
};

}

#endif //FIL_RNG_HH
