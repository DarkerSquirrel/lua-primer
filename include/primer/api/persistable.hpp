//  (C) Copyright 2015 - 2018 Christopher Beck

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/***
 * Class persistable is the base class which sets up an interface to use eris.
 *
 * API_FEATURES are registered as member variables, using a macro.
 * The persistable class exposes protected static member functions:

   void initialize_api(lua_State *);
   void persist(lua_State *, std::string &);
   void unpersist(lua_State *, const std::string &);

   initialize_api: Ask each feature to initialize itself in the given lua state.
   persist:        - Create a permanent objects table by asking each feature to
                     register its permanent objects ("on_persist" method).
                   - Create a target table, consisting of the global table, and
                     any auxiliary objects created by the features.
                     ("on_serialize" method)
                   - Invoke eris and serialize the result into the given string
                     buffer.
   unpersist:      - Create a (reversed) permanent objects table by asking each
                     feature to register its permanent objects. ("on_unpersist")
                   - Recreate the target table from the persisted string, using
                     eris.
                   - Install the reconstructed globals table, and ask each
                     feature to restore itself ("on_deserialize" method).

 *
 * The class persistable has no built-in member variables, it only provides
 * typedefs and static methods. The lua_State belongs to you, it is your job
 * to create it, and then explicitly initalize the api.
 */

#include <primer/base.hpp>

PRIMER_ASSERT_FILESCOPE;

#include <primer/eris.hpp>

#include <primer/api/feature.hpp>
#include <primer/api/init_caches.hpp>
#include <primer/detail/rank.hpp>
#include <primer/detail/typelist.hpp>
#include <primer/detail/typelist_iterator.hpp>
#include <primer/support/asserts.hpp>
#include <primer/support/lua_reader_writer.hpp>

namespace primer {

namespace api {

/***
 * Actual callback_registrar template
 */

template <typename T>
class persistable {

protected:
  typedef T primer_persistable_type;

  // detail, typelist assembly
  static inline detail::TypeList<> GetApiFeatures(primer::detail::Rank<0>);

  static constexpr int maxFeatures = 100;

#define GET_API_FEATURES                                                       \
  decltype(primer_persistable_type::GetApiFeatures(                            \
    primer::detail::Rank<maxFeatures>{}))

private:
  // Apply a visitor to the feature_list
  template <typename V>
  void visit_features(V && v) {
    using helper_t = detail::typelist_iterator<GET_API_FEATURES>;
    helper_t::apply_visitor(std::forward<V>(v), *static_cast<T *>(this));
  }

  void make_persist_table(lua_State * L) {
    lua_newtable(L);

    PRIMER_ASSERT_STACK_NEUTRAL(L);
    this->visit_features(on_persist_table_visitor{L});
  }

  void make_unpersist_table(lua_State * L) {
    lua_newtable(L);

    PRIMER_ASSERT_STACK_NEUTRAL(L);
    this->visit_features(on_unpersist_table_visitor{L});
  }

  static constexpr const char * global_table_field_name = "_G";

  void make_target_table(lua_State * L) {
    lua_newtable(L);
    // Store global table in the target table at position _G
    lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    lua_setfield(L, -2, global_table_field_name);

    PRIMER_ASSERT_STACK_NEUTRAL(L);
    this->visit_features(on_serialize_visitor{L});
  }

  void consume_target_table(lua_State * L) {
    PRIMER_ASSERT_TABLE(L);
    // Restore the persisted global table
    lua_getfield(L, -1, global_table_field_name);
    lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);

    {
      // TODO: This assertion is probably bad, since
      // these functions may raise lua errors and probably should be allowed
      // to leave the stack however. If lua is compiled as C++ then the dtor
      // here will be called.
      PRIMER_ASSERT_STACK_NEUTRAL(L);
      this->visit_features(on_deserialize_visitor{L});
    }
    lua_pop(L, 1);
  }

  // These impl functions may raise lua errors, but they should not throw
  // exceptions.
  void initialize_api_impl(lua_State * L) {
    primer::api::init_caches(L);
    this->visit_features(on_init_visitor{L});
  }

  void persist_impl(lua_State * L, std::string & buffer) {
    this->make_persist_table(L);
    this->make_target_table(L);

    buffer.resize(0);
    eris_dump(L, detail::trivial_string_writer, &buffer); // [_persist] [target]
  }

  void unpersist_impl(lua_State * L, const std::string & buffer) {
    this->make_unpersist_table(L); // [_unpersist]

    detail::reader_helper rh{buffer};
    eris_undump(L, detail::trivial_string_reader, &rh); // [_unpersist] [target]

    lua_remove(L, 1); // [target]
    this->consume_target_table(L);
  }

protected:
  /***
   * Forward-facing interface for derived classes. Initialization / persistance.
   */

  expected<void> initialize_api(lua_State * L) {
    PRIMER_ASSERT_STACK_NEUTRAL(L);

#ifdef PRIMER_DEBUG
    // Set debugging mode for eris
    lua_pushboolean(L, true);
    eris_set_setting(L, "path", -1);
    lua_pop(L, 1);
#endif

    return cpp_pcall<0>(L, [&L, this]() { this->initialize_api_impl(L); });
  }

  expected<void> persist(lua_State * L, std::string & buffer) {
    lua_settop(L, 0);

    expected<void> result =
      cpp_pcall<0>(L, [&L, &buffer, this]() { this->persist_impl(L, buffer); });

    lua_settop(L, 0);

    return result;
  }

  expected<void> unpersist(lua_State * L, const std::string & buffer) {
    lua_settop(L, 0);

    expected<void> result = cpp_pcall<0>(
      L, [&L, &buffer, this]() { this->unpersist_impl(L, buffer); });

    lua_settop(L, 0);

    return result;
  }
};

} // end namespace api

} // end namespace primer

/***
 * Macros to declare & register new callbacks
 */

#define API_FEATURE(TYPE, NAME)                                                \
  TYPE NAME;                                                                   \
  static constexpr const char * feature_name_##NAME() { return #NAME; }        \
  static_assert(primer::api::has_on_init_method<TYPE>::value,                  \
                "Missing on_init method!");                                    \
  static_assert(primer::api::has_on_persist_table_method<TYPE>::value,         \
                "Missing on_persist_table method!");                           \
  static_assert(primer::api::has_on_unpersist_table_method<TYPE>::value,       \
                "Missing on_unpersist_table method!");                         \
  static_assert(primer::api::is_feature<TYPE>::value,                          \
                "Type does not meet criteria for an API_FEATURE!");            \
  static inline primer::detail::                                               \
    Append_t<GET_API_FEATURES,                                                 \
             primer::api::                                                     \
               ptr_to_member<primer_persistable_type, TYPE,                    \
                             &primer_persistable_type::NAME,                   \
                             &primer_persistable_type::feature_name_##NAME>>   \
      GetApiFeatures(primer::detail::Rank<GET_API_FEATURES::size + 1>);        \
  static_assert(true, "")
