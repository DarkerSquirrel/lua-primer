//  (C) Copyright 2015 - 2018 Christopher Beck

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/***
 * How to push standard sequence objects like `std::vector`, `std::array` to the
 * stack, as a table. This is a collection common base classes using a template
 * parameter.
 */

#include <primer/base.hpp>

PRIMER_ASSERT_FILESCOPE;

#include <primer/detail/max_int.hpp>
#include <primer/detail/move_assign_noexcept.hpp>
#include <primer/detail/type_traits.hpp>
#include <primer/error_capture.hpp>
#include <primer/lua.hpp>
#include <primer/support/asserts.hpp>
#include <primer/traits/push.hpp>
#include <primer/traits/read.hpp>

#include <string>
#include <type_traits>
#include <utility>

namespace primer {
namespace container {

template <typename T>
struct push_seq_helper {
  using value_type = typename T::value_type;

  static void to_stack(lua_State * L, const T & seq) {
    int n = static_cast<int>(seq.size());
    lua_createtable(L, n, 0);

    PRIMER_ASSERT_STACK_NEUTRAL(L);
    for (int i = 0; i < n; ++i) {
      traits::push<value_type>::to_stack(L, seq[i]);
      lua_rawseti(L, -2, (i + 1));
    }
  }
  static constexpr int stack_space_needed{
    1 + traits::push<value_type>::stack_space_needed};
};

// For dynamically sized sequences, like std::vector
template <typename T>
struct read_seq_helper {
  using value_type = remove_cv_t<typename T::value_type>;

  PRIMER_STATIC_ASSERT(std::is_nothrow_constructible<T>::value,
                       "sequence type must be nothrow default constructible");
  PRIMER_STATIC_ASSERT(std::is_nothrow_move_constructible<value_type>::value,
                       "value type must be nothrow move constructible");

  // Reserve, if possible
  // Assume that it has same semantics as std::vector
  template <typename U, typename ENABLE = void>
  struct reserve_helper {
    static void reserve(U &, int) {}
  };

  template <typename U>
  struct reserve_helper<U, enable_if_t<std::is_same<decltype(std::declval<U>()
                                                               .reserve(0)),
                                                    decltype(
                                                      std::declval<U>().reserve(
                                                        0))>::value>> {
    static void reserve(U & u, int n) { u.reserve(n); }
  };

  static expected<T> from_stack(lua_State * L, int idx) {
    PRIMER_ASSERT_STACK_NEUTRAL(L);

    expected<T> result{};

    idx = lua_absindex(L, idx);
    if (!lua_istable(L, idx)) {
      result = primer::arg_error(L, idx, "table");
    } else {
      int n = lua_rawlen(L, idx);

      PRIMER_TRY_BAD_ALLOC { reserve_helper<T>::reserve(*result, n); }
      PRIMER_CATCH_BAD_ALLOC { result = primer::error::bad_alloc(); }

      for (int i = 0; (i < n) && result; ++i) {
        lua_rawgeti(L, idx, i + 1);
        if (auto object = traits::read<value_type>::from_stack(L, -1)) {
          PRIMER_TRY_BAD_ALLOC { result->emplace_back(std::move(*object)); }
          PRIMER_CATCH_BAD_ALLOC { result = primer::error::bad_alloc(); }
        } else {
          result = std::move(object.err());
          result.err().prepend_error_line("In index [", i + 1, "],");
        }
        lua_pop(L, 1);
      }
    }

    return result;
  }
  static constexpr int stack_space_needed{
    1 + traits::read<value_type>::stack_space_needed};
};

// For fixed sized sequences, like std::array
template <typename T>
struct read_fixed_seq_helper {
  using value_type = remove_cv_t<typename T::value_type>;

  PRIMER_STATIC_ASSERT(std::is_nothrow_constructible<T>::value,
                       "sequence type must be nothrow default constructible");

  static expected<T> from_stack(lua_State * L, int idx) {
    PRIMER_ASSERT_STACK_NEUTRAL(L);

    expected<T> result{};

    idx = lua_absindex(L, idx);
    if (!lua_istable(L, idx)) { result = primer::arg_error(L, idx, "table"); }

    int n = static_cast<int>(result->size());
    {
      int m = lua_rawlen(L, idx);
      if (m > n) {
        result = primer::error{"Too many elements, found ", m, " expected ", n};
      }
    }

    for (int i = 0; (i < n) && result; ++i) {
      lua_rawgeti(L, idx, i + 1);
      if (auto object = traits::read<value_type>::from_stack(L, -1)) {
        detail::move_assign_noexcept((*result)[i], std::move(*object));
      } else {
        result =
          std::move(object.err().prepend_error_line("In index [", i + 1, "],"));
      }
      lua_pop(L, 1);
    }

    return result;
  }
  static constexpr int stack_space_needed{
    1 + traits::read<value_type>::stack_space_needed};
};

} // end namespace container
} // end namespace primer
