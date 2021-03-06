//  (C) Copyright 2015 - 2018 Christopher Beck

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/***
 * Facilities to dispatch member function calls to a C++ object which has been
 * declared as userdata. In this case, we recover the object pointer from the
 * lua stack.
 */

#include <primer/base.hpp>

PRIMER_ASSERT_FILESCOPE;

#include <primer/adapt.hpp>
#include <primer/lua.hpp>
#include <primer/support/asserts.hpp>
#include <primer/userdata.hpp>

namespace primer {

/***
 * Dispatcher primary template. Constructs the delegate, falling back to the
 * basic adaptor if possible.
 *
 * Template parameter T represents the userdata type. It is explicitly specified
 * for safety. You will get a type mismatch if it does not match the base type
 * of the member pointer.
 */

template <typename T, typename F, F f>
struct userdata_dispatcher : public adapt<F, f> {};

/***
 * Specialize for a member function
 * When a method of userdata is called, lua passes the userdata object before
 * all the other parameters. So we just need to adapt signature
 *  R(T::*)(lua_State *, Args...)
 * to
 *  R (lua_State *, T &, Args...)
 */

template <typename T, typename R, typename... Args,
          R (T::*target_func)(lua_State *, Args...)>
struct userdata_dispatcher<T, R (T::*)(lua_State *, Args...), target_func> {

  static R dispatch_target(lua_State * L, T & t, Args... args) {
    return (t.*target_func)(L, std::forward<Args>(args)...);
  }

  static int adapted(lua_State * L) {
    using helper_t = adapt<R (*)(lua_State *, T &, Args...), dispatch_target>;
    return helper_t::adapted(L);
  }
};

/***
 * Const member functions
 */

template <typename T, typename R, typename... Args,
          R (T::*target_func)(lua_State *, Args...) const>
struct userdata_dispatcher<T, R (T::*)(lua_State *, Args...) const,
                           target_func> {

  static R dispatch_target(lua_State * L, const T & t, Args... args) {
    return (t.*target_func)(L, std::forward<Args>(args)...);
  }

  static int adapted(lua_State * L) {
    using helper_t =
      adapt<R (*)(lua_State *, const T &, Args...), dispatch_target>;
    return helper_t::adapted(L);
  }
};

/***
 * Specialize for member functions of "raw" type, i.e. int (T::*)(lua_State *).
 * Adapt won't know what to do with `int (lua_State *, T &)`.
 */
template <typename T, int (T::*target_func)(lua_State *)>
struct userdata_dispatcher<T, int (T::*)(lua_State *), target_func> {
  static int adapted(lua_State * L) {
    if (T * t = primer::test_udata<T>(L, 1)) {
      return (t->*target_func)(L);
    } else {
      return luaL_error(L, "bad argument #%d (Expected userdata of type '%s')",
                        1, primer::udata_name<T>());
    }
  }
};

/***
 * Const
 */

template <typename T, int (T::*target_func)(lua_State *) const>
struct userdata_dispatcher<T, int (T::*)(lua_State *) const, target_func> {
  static int adapted(lua_State * L) {
    if (T * t = primer::test_udata<T>(L, 1)) {
      return (t->*target_func)(L);
    } else {
      return luaL_error(L, "bad argument #%d (Expected userdata of type '%s')",
                        1, primer::udata_name<T>());
    }
  }
};

#define PRIMER_ADAPT_USERDATA(t, f)                                            \
  &primer::userdata_dispatcher<t, decltype(f), f>::adapted

} // end namespace primer
