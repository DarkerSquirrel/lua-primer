//  (C) Copyright 2015 - 2016 Christopher Beck

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/***
 * Implement the "as" method. This relies on primer::read, and
 * primer::traits::read is specialized for lua_ref... so this must be an
 * out-of-line definition
 *
 * Actually we implement both read and push traits here also.
 */

#include <primer/base.hpp>

PRIMER_ASSERT_FILESCOPE;

#include <primer/lua.hpp>

#include <primer/cpp_pcall.hpp>
#include <primer/error.hpp>
#include <primer/expected.hpp>
#include <primer/lua_ref.hpp>
#include <primer/maybe_int.hpp>
#include <primer/push.hpp>
#include <primer/read.hpp>

namespace primer {

namespace traits {

template <>
struct push<primer::lua_ref> {
  static void to_stack(lua_State * L, const primer::lua_ref & r) { r.push(L); }
  static constexpr maybe_int stack_space_needed{1};
};

// lua_ref
// Sort of a poor man's variant type, on read
template <>
struct read<lua_ref> {
  static expected<lua_ref> from_stack(lua_State * L, int idx) {
    expected<lua_ref> result{default_construct_in_place_tag{}};

    if (!lua_isnoneornil(L, idx)) {
      lua_pushvalue(L, idx);
      auto ok = mem_pcall<1>(L, &impl, L, result);
      if (!ok) { result = ok.err(); }
    }

    return result;
  }
  static constexpr maybe_int stack_space_needed{1};

  // This can cause memory allocation failure
  static void impl(lua_State * L, expected<lua_ref> & result) {
    result = lua_ref{L};
  }
};

} // end namespace traits

template <typename T>
expected<T> lua_ref::as() const noexcept {
  expected<T> result{primer::error::cant_lock_vm()};
  // ^ hoping for small string optimization here
  if (lua_State * L = this->push()) {
    result = primer::read<T>(L, -1);
    lua_pop(L, 1);
  }
  return result;
}

} // end namespace primer
