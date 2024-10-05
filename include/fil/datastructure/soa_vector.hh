
// - AGPL
// or
// - Subscription license for commercial usage (without requirement of licensing propagation).
//   please contact ballandfys@protonmail.com for additional information about this subscription commercial licensing.
//
// Copyright (c) 2022-2024.
// Created by fys on 05.10.24.
//
// In the case no license has been purchased for the use (modification or distribution in any way) of the software stack
// the APGL license is applying.

//
// Created by fys on 05.10.24.
//

#ifndef FIL_SOA_VECTOR_HH
#define FIL_SOA_VECTOR_HH

#include <tuple>
#include <vector>

namespace fil::soa {
//
//template<typename ...Ts>
//class soa_vector {
//	static constexpr std::index_sequence_for<Ts...> indexes{}; //!< indexes used for iteration over the tuple data container
//
// public:
//   // row should become a custom implementation in order to provide additional features afterward
//   using row = std::tuple<Ts&...>; //!< rows are simply
//
//   row operator[](std::size_t i) {
//	  // functor to iterate through the indexes sequence and return a raw of a reference over all the elements
//	  // in the data storage for the given index i.
//	  auto access = []<std::size_t ... Is>([[maybe_unused]]std::index_sequence<Ts...>) {
//		 return row{std::get<Is>(storage_)[i]...};
//	  };
//	  return std::invoke(access, indexes);
//   }
//
// private:
//   std::tuple<std::vector<Ts>...> storage_; //!< arrays of data are stored in this tuple of container
//
//};

}

#endif//FIL_SOA_VECTOR_HH
