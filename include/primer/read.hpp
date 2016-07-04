//  (C) Copyright 2015 - 2016 Christopher Beck

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/***
 * General purpose "read from stack" interface.
 * Backed up by the 'read' trait, see primer/traits/read.hpp
 */

#include <primer/base.hpp>

PRIMER_ASSERT_FILESCOPE;

#include <primer/traits/read.hpp>
#include <primer/expected.hpp>
#include <primer/detail/maybe_number.hpp>
#include <primer/support/asserts.hpp>

namespace primer {

//[ primer_read
template <typename T>
expected<T> read(lua_State * L, int index) {
  PRIMER_ASSERT_STACK_NEUTRAL(L);
  return ::primer::traits::read<T>::from_stack(L, index);
}
//]

//[ primer_stack_space_for_read
template <typename T>
constexpr detail::maybe_number stack_space_for_read() {
  return detail::stack_space_needed<::primer::traits::read<T>>::value;
}

} // end namespace primer
