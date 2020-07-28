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

#ifndef FIL_FINITE_STATE_MACHINE_HH
#define FIL_FINITE_STATE_MACHINE_HH

#include <cstdint>
#include <functional>
#include <tuple>
#include <vector>

namespace fil {

template<typename State>
class state_machine {
   static constexpr std::uint32_t BEFORE = 0;
   static constexpr std::uint32_t AFTER = 1;
   static constexpr std::uint32_t PREDICATE = 2;

   using state_type = State;
   using predicate = std::function<bool()>;
   using callback = std::function<void(state_type)>;

 public:
   state_machine(state_type st)
	   : _current_state(st) {}

   [[nodiscard]] state_type advance() {

	  std::uint32_t prevent_loop = 0;

	  for (std::uint32_t i = 0; i < _transitions.size(); ++i) {
		 if (std::get<BEFORE>(_transitions.at(i)) == _current_state && std::get<PREDICATE>(_transitions.at(i))()) {

			if (i < _on_entry_callbacks.size()) {
			   auto cb = _on_entry_callbacks.at(i);
			   if (cb) {
				  cb(_current_state);
			   }
			}

			_current_state = std::get<AFTER>(_transitions.at(i));

			// security in order to prevent infinite looping over always true predicates
			if (++prevent_loop >= _transitions.size()) {
			   return _current_state;
			}
			// reset 0 in order to retry previous transition as the current one succeed
			i = 0;
		 }
	  }
	  return _current_state;
   }

   void add_transition(state_type before, state_type after, predicate&& pred) {
	  reserve_callbacks(before);
	  reserve_callbacks(after);
	  _transitions.emplace_back(std::make_tuple(before, after, std::forward<predicate>(pred)));
   }

   void on_exit(state_type state_entry, callback&& on_exit) {
	  reserve_callbacks(state_entry);
	  _on_entry_callbacks[index_state(state_entry)] = std::forward<callback>(on_exit);
   }

   [[nodiscard]] state_type current_state() const { return _current_state; }

 private:
   std::uint32_t
   index_state(State state) {
	  return static_cast<std::uint32_t>(state);
   }

   void reserve_callbacks(state_type state) {
	  const std::uint32_t index = index_state(state);
	  if (index > _on_entry_callbacks.size()) {
		 _on_entry_callbacks.reserve(index);
	  }
   }

 private:
   state_type _current_state;

   std::vector<std::tuple<state_type, state_type, predicate>> _transitions;
   std::vector<callback> _on_entry_callbacks;
};

}// namespace fil

#endif//FIL_FINITE_STATE_MACHINE_HH
