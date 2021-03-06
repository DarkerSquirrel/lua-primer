#include <primer/primer.hpp>

#include "test_harness/test_harness.hpp"
#include <cassert>
#include <iostream>
#include <string>

using uint = unsigned int;

namespace {
//[ primer_expected_example
using primer::expected;

expected<std::string>
foo(expected<int> e) {
  if (e) {
    if (*e >= 7) {
      return std::string{"woof!"};
    } else {
      return primer::error{"bad doggie!"};
    }
  } else {
    return e.err();
  }
}

void
test_primer_expected() {
  auto result = foo(6);
  assert(!result);

  auto result2 = foo(7);
  assert(result2);
  assert(*result2 == "woof!");

  auto result3 = foo(primer::error("404"));
  assert(!result3);
  assert(result3.err().what() == std::string{"404"});

  assert(result3.err().str() == "404");
  assert(result3.err().c_str() == result3.err().what());
}
//]
} // end anonymous namespace

UNIT_TEST(test_primer_expected) { test_primer_expected(); }

template <typename T>
void
test_push_type(lua_State * L, T t, int expected, int line) {
  CHECK_STACK(L, 0);
  primer::push(L, t);
  CHECK_STACK(L, 1);
  test_top_type(L, expected, line);
  lua_pop(L, 1);
}

UNIT_TEST(simple_type_push) {
  lua_raii L;

  test_push_type<int>(L, 1, LUA_TNUMBER, __LINE__);
  test_push_type<int>(L, -101, LUA_TNUMBER, __LINE__);
  test_push_type<uint>(L, 9999, LUA_TNUMBER, __LINE__);
  test_push_type<const char *>(L, "asdf", LUA_TSTRING, __LINE__);
  test_push_type<std::string>(L, "jkl;", LUA_TSTRING, __LINE__);
  test_push_type<primer::nil_t>(L, primer::nil_t{}, LUA_TNIL, __LINE__);
  test_push_type<float>(L, 1.5f, LUA_TNUMBER, __LINE__);
  test_push_type<float>(L, 10000.5f, LUA_TNUMBER, __LINE__);
  test_push_type<bool>(L, true, LUA_TBOOLEAN, __LINE__);
  test_push_type<bool>(L, false, LUA_TBOOLEAN, __LINE__);
}

void
test_truthy(lua_State * L, bool expected, int line) {
  CHECK_STACK(L, 1);
  auto result = primer::read<primer::truthy>(L, 1);
  TEST_EXPECTED(result);
  TEST(result->value == expected, "Expected '" << (expected ? "true" : "false")
                                               << "'. Line: " << line);
  lua_pop(L, 1);
}

UNIT_TEST(read_truthy) {
  lua_raii L;

  {
    lua_pushnil(L);
    test_truthy(L, false, __LINE__);

    lua_pushboolean(L, true);
    test_truthy(L, true, __LINE__);

    lua_pushboolean(L, false);
    test_truthy(L, false, __LINE__);

    lua_pushinteger(L, 5);
    test_truthy(L, true, __LINE__);

    lua_pushstring(L, "asdf");
    test_truthy(L, true, __LINE__);

    lua_newtable(L);
    test_truthy(L, true, __LINE__);
  }
}

void
test_stringy(lua_State * L, std::string expected, int line) {
  CHECK_STACK(L, 1);
  auto result = primer::read<primer::stringy>(L, 1);
  TEST_EXPECTED(result);
  TEST(result->value == expected, "Expected '" << expected
                                               << "'. Line: " << line);
  lua_pop(L, 1);
}

void
test_not_stringy(lua_State * L, int line) {
  CHECK_STACK(L, 1);
  auto result = primer::read<primer::stringy>(L, 1);
  TEST(!result, "Expected failure, found '" << result->value
                                            << "'. Line: " << line);
  lua_pop(L, 1);
}

int
dummy_to_string_method(lua_State * L) {
  lua_pushstring(L, "asdf");
  return 1;
}

UNIT_TEST(read_stringy) {
  lua_raii L;

  {
    lua_pushnil(L);
    test_not_stringy(L, __LINE__);

    lua_pushboolean(L, true);
    test_not_stringy(L, __LINE__);

    lua_pushboolean(L, false);
    test_not_stringy(L, __LINE__);

    lua_pushinteger(L, 5);
    test_stringy(L, "5", __LINE__);

    lua_pushstring(L, "asdf");
    test_stringy(L, "asdf", __LINE__);

    lua_newtable(L);
    test_not_stringy(L, __LINE__);

    lua_newtable(L);
    lua_newtable(L);
    lua_setmetatable(L, -2);
    test_not_stringy(L, __LINE__);

    lua_newtable(L);
    lua_newtable(L);
    lua_pushcfunction(L, dummy_to_string_method);
    lua_setfield(L, -2, "__tostring");
    lua_setmetatable(L, -2);
    test_stringy(L, "asdf", __LINE__);
  }
}

template <typename T>
void
test_roundtrip_value(lua_State * L, T t, int line) {
  CHECK_STACK(L, 0);

  primer::push(L, t);

  CHECK_STACK(L, 1);

  auto r = primer::read<T>(L, 1);
  TEST(r, "Could not recover value '" << t << "'. line: " << line);

  TEST_EQ(*r, t);

  CHECK_STACK(L, 1);

  lua_pop(L, 1);
}

UNIT_TEST(roundtrip_simple) {
  lua_raii L;

  test_roundtrip_value<int>(L, 1, __LINE__);
  test_roundtrip_value<int>(L, 2, __LINE__);
  test_roundtrip_value<int>(L, 3, __LINE__);
  test_roundtrip_value<int>(L, -19, __LINE__);
  test_roundtrip_value<int>(L, 657, __LINE__);

  test_roundtrip_value<std::string>(L, "foo", __LINE__);
  test_roundtrip_value<std::string>(L, "bar", __LINE__);
  test_roundtrip_value<std::string>(L, "", __LINE__);

  test_roundtrip_value<uint>(L, 0, __LINE__);
  test_roundtrip_value<uint>(L, 27, __LINE__);
  test_roundtrip_value<uint>(L, 3, __LINE__);
  test_roundtrip_value<uint>(L, std::numeric_limits<int>::max(), __LINE__);

  test_roundtrip_value<bool>(L, true, __LINE__);
  test_roundtrip_value<bool>(L, false, __LINE__);

  test_roundtrip_value<float>(L, 1, __LINE__);
  test_roundtrip_value<float>(L, 2, __LINE__);
  test_roundtrip_value<float>(L, 3, __LINE__);
  test_roundtrip_value<float>(L, 100.5f, __LINE__);
  test_roundtrip_value<float>(L, -97.75f, __LINE__);
}

template <typename U, typename T>
void
test_type_safety(lua_State * L, T t, int line) {
  CHECK_STACK(L, 0);

  primer::push(L, t);

  CHECK_STACK(L, 1);

  auto r = primer::read<U>(L, 1);

  if (r) {
    throw test_exception{"Unexpected conversion was permitted by primer: line "
                         + std::to_string(line)};
  }

  CHECK_STACK(L, 1);
  lua_pop(L, 1);
}

UNIT_TEST(simple_type_safety) {
  lua_raii L;

  test_type_safety<std::string>(L, int{1}, __LINE__);
  test_type_safety<int>(L, std::string{"foo"}, __LINE__);
  test_type_safety<bool>(L, "bar", __LINE__);
  test_type_safety<int>(L, 7.5f, __LINE__);
  test_type_safety<bool>(L, int{2}, __LINE__);
  test_type_safety<unsigned int>(L, int{-1}, __LINE__);
  test_type_safety<unsigned int>(L, int{-99}, __LINE__);
  test_type_safety<std::string>(L, 7.5f, __LINE__);
  test_type_safety<int>(L, primer::nil_t{}, __LINE__);
}

/***
 * Test primer adapt
 */

namespace {
int test_result_one;
float test_result_two;
std::string test_result_three;

primer::result
test_func_one(lua_State * L, int i, float d, std::string s) {
  test_result_one = i;
  test_result_two = d;
  test_result_three = s;
  lua_pushboolean(L, true);
  return 1;
}
} // end anonymous namespace

UNIT_TEST(adapt_one) {
  lua_raii L;

  lua_CFunction func = PRIMER_ADAPT(&test_func_one);

  lua_pushcfunction(L, func);
  lua_pushinteger(L, 3);
  lua_pushnumber(L, 5.5f);
  lua_pushstring(L, "asdf");

  TEST_LUA_OK(L, lua_pcall(L, 3, 1, 0));
  CHECK_STACK(L, 1);
  test_top_type(L, LUA_TBOOLEAN, __LINE__);
  TEST_EQ(lua_toboolean(L, 1), true);
  TEST_EQ(test_result_one, 3);
  TEST_EQ(test_result_two, 5.5f);
  TEST_EQ(test_result_three, "asdf");

  lua_pop(L, 1);
  CHECK_STACK(L, 0);

  lua_pushcfunction(L, func);
  TEST_EQ(LUA_ERRRUN, lua_pcall(L, 0, 1, 0));

  CHECK_STACK(L, 1);
  lua_pop(L, 1);

  lua_pushcfunction(L, func);
  lua_pushinteger(L, 7);
  lua_pushnumber(L, -17.5f);
  lua_pushstring(L, "jkl;");

  TEST_LUA_OK(L, lua_pcall(L, 3, 1, 0));
  CHECK_STACK(L, 1);
  TEST_EQ(test_result_one, 7);
  TEST_EQ(test_result_two, -17.5f);
  TEST_EQ(test_result_three, "jkl;");
  lua_pop(L, 1);

  lua_pushcfunction(L, func);
  lua_pushstring(L, "jkl;");
  lua_pushinteger(L, 7);
  lua_pushnumber(L, -17.5f);

  TEST_EQ(LUA_ERRRUN, lua_pcall(L, 3, 1, 0));

  CHECK_STACK(L, 1);
  lua_pop(L, 1);

  lua_pushcfunction(L, func);
  lua_pushnumber(L, 7);
  lua_pushstring(L, "jkl;");
  lua_pushnumber(L, 17.5f);

  TEST_EQ(LUA_ERRRUN, lua_pcall(L, 3, 1, 0));

  CHECK_STACK(L, 1);
  lua_pop(L, 1);

  lua_pushcfunction(L, func);
  lua_pushnumber(L, 7);
  lua_pushstring(L, "jkl;");

  TEST_EQ(LUA_ERRRUN, lua_pcall(L, 2, 1, 0));

  CHECK_STACK(L, 1);
  lua_pop(L, 1);

  lua_pushcfunction(L, func);
  lua_pushnumber(L, 7);
  lua_pushnil(L);
  lua_pushstring(L, "jkl;");

  TEST_EQ(LUA_ERRRUN, lua_pcall(L, 3, 1, 0));

  CHECK_STACK(L, 1);
  lua_pop(L, 1);
}

namespace {

primer::result
test_func_two(lua_State *) {
  return primer::error("foo");
}
}

UNIT_TEST(adapt_two) {
  lua_raii L;

  lua_CFunction func = PRIMER_ADAPT(&test_func_two);

  lua_pushcfunction(L, func);
  TEST_EQ(LUA_ERRRUN, lua_pcall(L, 0, 0, 0));

  CHECK_STACK(L, 1);
  lua_pop(L, 1);

  luaL_requiref(L, "", &luaopen_base, 1);
  lua_pop(L, 1);

  lua_pushcfunction(L, func);
  lua_setglobal(L, "f");

  TEST_LUA_OK(L, luaL_loadstring(L, "pcall(f()); return true"));
  TEST_EQ(LUA_ERRRUN, lua_pcall(L, 0, 1, 0));

  TEST_EQ(lua_type(L, 1), LUA_TSTRING);
  TEST_EQ(std::string{"foo"}, lua_tostring(L, 1));

  CHECK_STACK(L, 1);
  lua_pop(L, 1);

  TEST_LUA_OK(L, luaL_loadstring(L, "pcall(f); return true"));
  TEST_LUA_OK(L, lua_pcall(L, 0, 1, 0));

  CHECK_STACK(L, 1);
  TEST_EQ(lua_type(L, 1), LUA_TBOOLEAN);
  TEST_EQ(true, lua_toboolean(L, 1));
}

namespace {

primer::result
test_func_three(lua_State * L, int i, int j, bool b) {
  if (i != 5) {
    primer::push(L, i);
    return 1;
  }

  if (j != 7) {
    primer::push(L, j);
    return primer::yield{1};
  }

  if (b) {
    return primer::error{"foo"};
  } else {
    const char * msg = "bar";
    primer::push(L, msg);
    return 1;
  }
}

} // end anonymous namespace

UNIT_TEST(adapt_three) {
  lua_raii L;

  lua_CFunction func = PRIMER_ADAPT(&test_func_three);

  {
    lua_State * T = lua_newthread(L);

    lua_pushcfunction(T, func);
    lua_pushinteger(T, 5);
    lua_pushinteger(T, 7);
    lua_pushboolean(T, false);

    TEST_LUA_OK(T, lua_resume(T, nullptr, 3));
    CHECK_STACK(T, 1);
    test_top_type(T, LUA_TSTRING, __LINE__);
    TEST_EQ(std::string{"bar"}, lua_tostring(T, 1));

    lua_pop(L, 1);
  }

  CHECK_STACK(L, 0);

  {
    lua_State * T = lua_newthread(L);

    lua_pushcfunction(T, func);
    lua_pushinteger(T, 3);
    lua_pushinteger(T, 7);
    lua_pushboolean(T, false);

    TEST_LUA_OK(T, lua_resume(T, nullptr, 3));
    CHECK_STACK(T, 1);
    test_top_type(T, LUA_TNUMBER, __LINE__);
    TEST_EQ(3, lua_tointeger(T, 1));

    lua_pop(L, 1);
  }

  CHECK_STACK(L, 0);

  {
    lua_State * T = lua_newthread(L);

    lua_pushcfunction(T, func);
    lua_pushinteger(T, 5);
    lua_pushinteger(T, 6);
    lua_pushboolean(T, false);

    TEST_EQ(LUA_YIELD, lua_resume(T, nullptr, 3));
    CHECK_STACK(T, 1);
    test_top_type(T, LUA_TNUMBER, __LINE__);
    TEST_EQ(6, lua_tointeger(T, 1));

    lua_pop(L, 1);
  }

  CHECK_STACK(L, 0);

  {
    lua_State * T = lua_newthread(L);

    lua_pushcfunction(T, func);
    lua_pushinteger(T, 5);
    lua_pushinteger(T, 7);
    lua_pushboolean(T, true);

    TEST_EQ(LUA_ERRRUN, lua_resume(T, nullptr, 3));

    CHECK_STACK(L, 1);
    CHECK_STACK(T, 5);

    test_type(L, 1, LUA_TTHREAD, __LINE__);
    test_type(T, 1, LUA_TNUMBER, __LINE__);
    test_type(T, 2, LUA_TNUMBER, __LINE__);
    test_type(T, 3, LUA_TBOOLEAN, __LINE__);
    test_type(T, 4, LUA_TSTRING, __LINE__);
    test_type(T, 5, LUA_TSTRING, __LINE__);

    TEST_EQ(lua_tostring(T, 4), std::string{"foo"});
    // Last item is the stack trace... this is a property of lua_resume.

    lua_pop(L, 1);
  }

  CHECK_STACK(L, 0);
}

#define WEAK_REF_TEST(X)                                                       \
  TEST(X, "Unexpected value for lua_state_ref. line: " << __LINE__)

UNIT_TEST(lua_state_ref_validity) {
  using primer::lua_state_ref;

  lua_state_ref r;
  lua_state_ref s;
  lua_state_ref t;

  WEAK_REF_TEST(!r);
  WEAK_REF_TEST(!s);
  WEAK_REF_TEST(!t);

  {
    lua_raii L;

    r = primer::obtain_state_ref(L);

    WEAK_REF_TEST(r);
    WEAK_REF_TEST(!s);
    WEAK_REF_TEST(!t);

    s = r;

    WEAK_REF_TEST(r);
    WEAK_REF_TEST(s);
    WEAK_REF_TEST(!t);

    lua_State * l = L;
    TEST_EQ(l, r.lock());
    TEST_EQ(l, s.lock());
  }

  WEAK_REF_TEST(!r);
  WEAK_REF_TEST(!s);
  WEAK_REF_TEST(!t);

  {
    lua_raii L;

    t = primer::obtain_state_ref(L);

    lua_State * l = L;
    TEST_EQ(l, t.lock());

    WEAK_REF_TEST(!r);
    WEAK_REF_TEST(!s);
    WEAK_REF_TEST(t);

    s = t;

    WEAK_REF_TEST(!r);
    WEAK_REF_TEST(s);
    WEAK_REF_TEST(t);

    // TEST_EQ(L, s.lock());

    primer::close_state_refs(L);

    WEAK_REF_TEST(!r);
    WEAK_REF_TEST(!s);
    WEAK_REF_TEST(!t);
  }

  WEAK_REF_TEST(!r);
  WEAK_REF_TEST(!s);
  WEAK_REF_TEST(!t);

  {
    lua_raii L;

    primer::close_state_refs(L);

    s = primer::obtain_state_ref(L);

    WEAK_REF_TEST(!r);
    WEAK_REF_TEST(!s);
    WEAK_REF_TEST(!t);

    r = s;

    WEAK_REF_TEST(!r);
    WEAK_REF_TEST(!s);
    WEAK_REF_TEST(!t);
  }

  WEAK_REF_TEST(!r);
  WEAK_REF_TEST(!s);
  WEAK_REF_TEST(!t);
}

UNIT_TEST(lua_ref) {
  lua_raii L;

  {
    primer::lua_ref dummy;
    TEST(!dummy, "expected empty state to evaluate false");
  }

  lua_pushstring(L, "asdf");
  primer::lua_ref foo{L};
  CHECK_STACK(L, 0);
  TEST(foo, "expected ref to be engaged");

  lua_pushstring(L, "jkl;");
  lua_pushnumber(L, 15);
  primer::lua_ref bar{L};
  CHECK_STACK(L, 1);
  primer::lua_ref baz{L};

  CHECK_STACK(L, 0);

  TEST(bar, "Expected ref to be engaged");
  TEST(baz, "Expected ref to be engaged");

  TEST(baz.push(L), "Expected push to succeed");
  CHECK_STACK(L, 1);
  test_top_type(L, LUA_TSTRING, __LINE__);
  TEST_EQ(std::string{"jkl;"}, lua_tostring(L, -1));
  lua_pop(L, 1);

  TEST(bar.push(), "Expected push to succeed");
  CHECK_STACK(L, 1);
  test_top_type(L, LUA_TNUMBER, __LINE__);
  TEST_EQ(15, lua_tonumber(L, -1));
  lua_pop(L, 1);

  TEST(foo.push(), "Expected push to succeed");
  CHECK_STACK(L, 1);
  test_top_type(L, LUA_TSTRING, __LINE__);
  TEST_EQ(std::string{"asdf"}, lua_tostring(L, -1));
  lua_pop(L, 1);

  // Test reset
  bar.reset();
  TEST(!bar.push(), "Expected ref to be in the empty state!");
  CHECK_STACK(L, 0);

  TEST(baz.push(), "Expected push to succeed");
  CHECK_STACK(L, 1);
  test_top_type(L, LUA_TSTRING, __LINE__);
  TEST_EQ(std::string{"jkl;"}, lua_tostring(L, -1));
  lua_pop(L, 1);

  // Move assign
  bar = std::move(baz);

  TEST(!baz.push(), "Expected ref that has been moved from to be empty!");
  CHECK_STACK(L, 0);

  TEST(bar.push(), "Expected push to succeed");
  CHECK_STACK(L, 1);
  test_top_type(L, LUA_TSTRING, __LINE__);
  TEST_EQ(std::string{"jkl;"}, lua_tostring(L, -1));
  lua_pop(L, 1);

  // Copy assign a disengaged
  baz = bar;

  TEST(bar.push(), "Expected push to succeed");
  CHECK_STACK(L, 1);
  test_top_type(L, LUA_TSTRING, __LINE__);
  TEST_EQ(std::string{"jkl;"}, lua_tostring(L, -1));
  lua_pop(L, 1);

  TEST(baz.push(), "Expected push to succeed");
  CHECK_STACK(L, 1);
  test_top_type(L, LUA_TSTRING, __LINE__);
  TEST_EQ(std::string{"jkl;"}, lua_tostring(L, -1));
  lua_pop(L, 1);

  // Copy assign an engaged
  bar = foo;

  TEST(bar.push(), "Expected push to succeed");
  CHECK_STACK(L, 1);
  test_top_type(L, LUA_TSTRING, __LINE__);
  TEST_EQ(std::string{"asdf"}, lua_tostring(L, -1));
  lua_pop(L, 1);

  TEST(foo.push(), "Expected push to succeed");
  CHECK_STACK(L, 1);
  test_top_type(L, LUA_TSTRING, __LINE__);
  TEST_EQ(std::string{"asdf"}, lua_tostring(L, -1));
  lua_pop(L, 1);

  // Close all refs
  primer::close_state_refs(L);
  TEST(!foo, "Expected all refs to be closed now!");
  TEST(!bar, "Expected all refs to be closed now!");
  TEST(!baz, "Expected all refs to be closed now!");
}

UNIT_TEST(lua_ref_examples) {
  //[ primer_example_ref
  lua_State * L = luaL_newstate();

  std::string foo{"foo"};
  primer::push(L, foo);
  assert(lua_gettop(L) == 1);

  primer::lua_ref ref{L};
  assert(lua_gettop(L) == 0);
  assert(ref);

  assert(ref.as<std::string>() && foo == *ref.as<std::string>());
  assert(!ref.as<int>());
  assert(!ref.as<bool>());
  assert(ref.as<primer::truthy>() && true == ref.as<primer::truthy>()->value);

  assert(ref.push());
  assert(lua_gettop(L) == 1);
  assert(lua_isstring(L, 1));
  assert(foo == lua_tostring(L, 1));

  lua_pop(L, 1);

  assert(ref);
  assert(ref.as<std::string>() && foo == *ref.as<std::string>());

  lua_close(L);

  assert(!ref);
  assert(!ref.as<std::string>());
  //]
}

primer::result
test_func_four(lua_State * L, int i, int j) {
  lua_pushinteger(L, i + j);
  lua_pushinteger(L, i - j);
  return 2;
}

UNIT_TEST(primer_call) {
  lua_raii L;

  lua_CFunction f1 = PRIMER_ADAPT(&test_func_one);

  {
    lua_pushcfunction(L, f1);
    auto result = primer::fcn_call_no_ret(L, 0);
    CHECK_STACK(L, 0);
    TEST(!result, "expected an error");
  }

  {
    lua_pushcfunction(L, f1);
    lua_pushinteger(L, 3);
    lua_pushnumber(L, 4.5f);
    lua_pushstring(L, "asdf");

    auto result = primer::fcn_call_no_ret(L, 3);
    CHECK_STACK(L, 0);
    TEST_EXPECTED(result);
  }

  lua_CFunction f2 = PRIMER_ADAPT(&test_func_two);

  {
    lua_pushcfunction(L, f2);
    lua_pushinteger(L, 3);
    auto result = primer::fcn_call_no_ret(L, 1);
    CHECK_STACK(L, 0);
    TEST(!result, "expected failure");
  }

  lua_CFunction f4 = PRIMER_ADAPT(&test_func_four);

  {
    lua_pushcfunction(L, f4);
    lua_pushinteger(L, 5);
    lua_pushinteger(L, 7);

    auto result = primer::fcn_call_one_ret(L, 2);

    CHECK_STACK(L, 0);
    TEST_EXPECTED(result);
    TEST(result->push(), "expected to be able to push");
    CHECK_STACK(L, 1);
    test_top_type(L, LUA_TNUMBER, __LINE__);
    TEST_EQ(lua_tointeger(L, 1), 12);
    lua_pop(L, 1);
  }

  {
    lua_pushcfunction(L, f4);
    lua_pushinteger(L, 19);
    lua_pushinteger(L, -5);

    auto result = primer::fcn_call_one_ret(L, 2);

    CHECK_STACK(L, 0);
    TEST_EXPECTED(result);

    auto as_int = result->as<int>();
    CHECK_STACK(L, 0);

    TEST_EXPECTED(as_int);
    TEST_EQ(14, *as_int);
  }
  CHECK_STACK(L, 0);
}

UNIT_TEST(primer_resume) {
  lua_raii L;

  using primer::expected;
  using primer::lua_ref;

  lua_CFunction f1 = PRIMER_ADAPT(&test_func_three);

  lua_State * T = lua_newthread(L);
  {
    TEST_EQ(LUA_OK, lua_status(T));

    lua_pushcfunction(T, f1);
    lua_pushinteger(T, 4);
    lua_pushinteger(T, 7);
    lua_pushboolean(T, true);

    expected<lua_ref> result = primer::resume_one_ret(T, 3);
    int code = lua_status(T);

    TEST_EQ(code, LUA_OK);
    TEST(result, "expected success");
    CHECK_STACK(T, 0);
    CHECK_STACK(L, 1);
    TEST(result->push(), "expected to be able to push");
    CHECK_STACK(T, 0);
    CHECK_STACK(L, 2);
    test_top_type(L, LUA_TNUMBER, __LINE__);
    TEST_EQ(lua_tonumber(L, 2), 4);
    lua_pop(L, 1);
  }

  {
    TEST_EQ(LUA_OK, lua_status(T));

    lua_pushcfunction(T, f1);
    lua_pushinteger(T, 5);
    lua_pushinteger(T, 6);
    lua_pushboolean(T, true);

    expected<lua_ref> result = primer::resume_one_ret(T, 3);
    int code = lua_status(T);

    TEST_EQ(code, LUA_YIELD);
    TEST(result, "expected success");
    CHECK_STACK(T, 0);
    TEST(result->push(T), "expected to be able to push");
    CHECK_STACK(T, 1);
    test_top_type(T, LUA_TNUMBER, __LINE__);
    TEST_EQ(lua_tonumber(T, 1), 6);
    lua_pop(T, 1);

    TEST_EQ(LUA_YIELD, lua_status(T));

    lua_pushstring(T, "asdf");
    result = primer::resume_one_ret(T, 1);
    code = lua_status(T);
    TEST_EQ(code, LUA_OK);

    CHECK_STACK(T, 0);
  }

  {
    TEST_EQ(LUA_OK, lua_status(T));

    lua_pushcfunction(T, f1);
    lua_pushinteger(T, 5);
    lua_pushinteger(T, 7);
    lua_pushboolean(T, true);

    expected<lua_ref> result = primer::resume_one_ret(T, 3);
    int code = lua_status(T);

    TEST_EQ(code, LUA_ERRRUN);
    TEST(!result, "expected an error message");

    CHECK_STACK(T, 0);
  }
}

namespace {

//[ primer_example_vec2i_defn
//` For example, suppose we have a simple vector type:
struct vec2i {
  int x;
  int y;
};
//]

} // end anonymous namespace

//[ primer_example_vec2i_push_trait
//` Primer could be taught to push `vec2i` objects as a table with
//` entries `t[1]` and `t[2]` using the following code:

namespace primer {
namespace traits {

template <>
struct push<vec2i> {
  static void to_stack(lua_State * L, const vec2i & v) {
    lua_newtable(L);
    lua_pushinteger(L, v.x);
    lua_rawseti(L, -2, 1);
    lua_pushinteger(L, v.y);
    lua_rawseti(L, -2, 2);
  }
  static constexpr int stack_space_needed = 2;
};

} // end namespace traits
} // end namespace primer

//]

UNIT_TEST(vec2i_push) {
  lua_raii L;

  luaL_requiref(L, "", &luaopen_base, 1);
  lua_pop(L, 1);

  //[ primer_example_vec2i_push_test
  //` The following code snippet shows how the pushed object looks to lua
  primer::push(L, vec2i{5, 3});
  luaL_loadstring(L,
                  "v = ...                          \n"
                  "assert(type(v) == 'table')       \n"
                  "assert(v[1] == 5)                \n"
                  "assert(v[2] == 3)                \n"
                  "assert(#v == 2)                  \n");

  lua_insert(L, -2); // put script beneath argument when using pcall
  assert(LUA_OK == lua_pcall(L, 1, 0, 0));
  //]
}

//[ primer_example_vec2i_read_trait
//`Primer could be taught to read `vec2i` objects, represented in lua
//`as a table with entries `t[1]` and `t[2]` using the following code:

namespace primer {
namespace traits {

template <>
struct read<vec2i> {
  static expected<vec2i> from_stack(lua_State * L, int index) {
    expected<vec2i> result;

    if (!lua_istable(L, index)) {
      result = primer::error("Expected a table, found ",
                             primer::describe_lua_value(L, index));
    } else {
      lua_rawgeti(L, index, 1);
      expected<int> t1 = read<int>::from_stack(L, -1);
      lua_pop(L, 1);

      if (!t1) {
        t1.err().prepend_error_line("In position [1]");
        result = std::move(t1.err());
      } else {

        lua_rawgeti(L, index, 2);
        expected<int> t2 = read<int>::from_stack(L, -1);
        lua_pop(L, 1);

        if (!t2) {
          t2.err().prepend_error_line("In position [2]");
          result = std::move(t2.err());
        } else {
          result = vec2i{*t1, *t2};
        }
      }
    }

    return result;
  }

  static constexpr int stack_space_needed = 1;
};

} // end namespace traits
} // end namespace primer
/*`
A few things to note about this code:


* The returned local variable is declared at the top, and only one return
statement exists, at the end. This ensures that named return value optimization
can take place.

* We avoid using the stack excessively. When a new value is pushed onto the
stack using `lua_rawgeti`, we read it immediately and then pop it. This means we
need only one extra stack space rather than two.

* One consequence of this is that we don't need to worry about the case that
`index` is negative. In general, one can use `lua_absindex` to convert negative
indices to positive indices. If the top of the stack is changing, then this may
be necessary for correctness.

* The `primer::error` constructor can take any number of strings, and
concatenates them together. `primer::describe_lua_value` is used to generate a
diagnostic message describing a value on the stack.

* The `primer::error` member function `prepend_error_line` is used to give
context to errors reported by subsidiary operations.

*/
//]

UNIT_TEST(vec2i_read) {
  lua_raii L;

  //[ primer_example_vec2i_read_test
  //` The following code snippet shows how this looks from lua's point of view
  luaL_loadstring(L, "return {7, 4}");
  assert(LUA_OK == lua_pcall(L, 0, 1, 0));
  assert(lua_gettop(L) == 1); // the table is now on top of the stack
  assert(lua_istable(L, 1));

  auto result = primer::read<vec2i>(L, 1);
  assert(result);
  assert(result->x == 7);
  assert(result->y == 4);
  //]
}

UNIT_TEST(coroutine) {
  lua_raii L;

  luaL_requiref(L, "", luaopen_base, 1);
  lua_pop(L, 1);

  luaopen_coroutine(L);
  lua_getfield(L, -1, "yield");
  TEST(lua_isfunction(L, -1), "couldn't find coroutine yield function");
  lua_setglobal(L, "yield");
  lua_pop(L, 1);

  CHECK_STACK(L, 0);

  const char * script =
    "return function(x, y)                               \n"
    "  while true do                                     \n"
    "    x, y = yield(x + y, x - y)                      \n"
    "  end                                               \n"
    "end                                                 \n";

  TEST_LUA_OK(L, luaL_loadstring(L, script));
  TEST_LUA_OK(L, primer::protected_call(L, 0, 1));

  primer::bound_function f{L};
  TEST(f, "expected to find a function");

  CHECK_STACK(L, 0);

  primer::coroutine c{f};
  TEST(c, "expected to find a corouitne");

  CHECK_STACK(L, 0);

  lua_pushinteger(L, 3);
  lua_pushinteger(L, 5);

  primer::lua_ref_seq a = primer::pop_stack(L);

  CHECK_STACK(L, 0);
  TEST_EQ(a.size(), 2);
  TEST(a[0], "expected a valid entry");
  TEST(a[1], "expected a valid entry");

  auto result = c.call(a);
  CHECK_STACK(L, 0);
  TEST_EXPECTED(result);
  TEST_EQ(result->size(), 2);
  TEST_EXPECTED(result->at(0).as<int>());
  TEST_EXPECTED(result->at(1).as<int>());
  TEST_EQ(*result->at(0).as<int>(), 8);
  TEST_EQ(*result->at(1).as<int>(), -2);
  CHECK_STACK(L, 0);

  a = std::move(*result);

  CHECK_STACK(L, 0);

  result = c.call(a);
  CHECK_STACK(L, 0);
  TEST_EXPECTED(result);
  TEST_EQ(result->size(), 2);
  TEST_EXPECTED(result->at(0).as<int>());
  TEST_EXPECTED(result->at(1).as<int>());
  TEST_EQ(*result->at(0).as<int>(), 6);
  TEST_EQ(*result->at(1).as<int>(), 10);
  CHECK_STACK(L, 0);

  a = std::move(*result);

  CHECK_STACK(L, 0);

  result = c.call(a);
  CHECK_STACK(L, 0);
  TEST_EXPECTED(result);
  TEST_EQ(result->size(), 2);
  TEST_EXPECTED(result->at(0).as<int>());
  TEST_EXPECTED(result->at(1).as<int>());
  TEST_EQ(*result->at(0).as<int>(), 16);
  TEST_EQ(*result->at(1).as<int>(), -4);
  CHECK_STACK(L, 0);

  a = std::move(*result);

  CHECK_STACK(L, 0);

  result = c.call(a);
  CHECK_STACK(L, 0);
  TEST_EXPECTED(result);
  TEST_EQ(result->size(), 2);
  TEST_EXPECTED(result->at(0).as<int>());
  TEST_EXPECTED(result->at(1).as<int>());
  TEST_EQ(*result->at(0).as<int>(), 12);
  TEST_EQ(*result->at(1).as<int>(), 20);
  CHECK_STACK(L, 0);

  a = std::move(*result);

  CHECK_STACK(L, 0);

  result = c.call("wrench");
  TEST(!result, "expected failure");
  TEST(!c, "expected dead coroutine");

  result = c.call(a);
  TEST(!result, "expected failure");
  TEST(!c, "expected dead coroutine");
}

// Test coroutine with lua_ref_seq interface
primer::result
yielder(lua_State *) {
  return primer::yield{0};
}

primer::result
returner(lua_State * L) {
  lua_pushinteger(L, 7);
  return 1;
}

UNIT_TEST(coroutine_two) {
  lua_raii L;

  lua_pushcfunction(L, PRIMER_ADAPT(&yielder));
  primer::bound_function f1{L};

  lua_pushcfunction(L, PRIMER_ADAPT(&returner));
  primer::bound_function f2{L};

  primer::coroutine c1{f1};
  primer::coroutine c2{f2};

  TEST(c1, "expected valid coroutine");
  TEST(c2, "expected valid coroutine");

  auto r1 = c1.call();
  TEST_EXPECTED(r1);
  TEST_EQ(0, r1->size());
  TEST(c1, "expected valid coroutine");

  auto r2 = c2.call();
  TEST_EXPECTED(r2);
  TEST_EQ(1, r2->size());
  TEST(!c2, "expected invalid coroutine");

  auto r3 = c1.call(*r2);
  TEST_EXPECTED(r3);
  TEST_EQ(1, r3->size());
  TEST(!c1, "expected invalid coroutine");

  auto i = r3->at(0).as<int>();
  TEST_EXPECTED(i);
  TEST_EQ(7, *i);
}

// This test catches a subtle issue regarding whether or not cpp_pcall
// messes up the stack when it returns.
UNIT_TEST(cpp_pcall_returns) {
  lua_raii L;

  lua_pushinteger(L, 2);
  lua_pushinteger(L, 3);

  // Use mem_pcall so that tests with lua as C++ and with lua as C go both ways
  primer::mem_pcall(L, [&]() {
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
  });

  TEST_EQ(4, lua_gettop(L));
  TEST_EQ(2, lua_tointeger(L, 1));
  TEST_EQ(3, lua_tointeger(L, 2));
  TEST_EQ(2, lua_tointeger(L, 1));
  TEST_EQ(3, lua_tointeger(L, 2));

  primer::mem_pcall<3>(L, [&]() { lua_pop(L, 2); });

  TEST_EQ(2, lua_gettop(L));
  TEST_EQ(2, lua_tointeger(L, 1));
  TEST_EQ(3, lua_tointeger(L, 2));

  primer::mem_pcall<2>(L, [&]() { lua_pop(L, 2); });

  TEST_EQ(0, lua_gettop(L));
}

//[ primer_raise_lua_error_decl

// This is a custom exception type, which is supposed to be handled by raising
// a lua error with this error message.
struct raise_lua_error : std::runtime_error {
  raise_lua_error(const std::string & str)
    : std::runtime_error(str) {}
};

// To make our own partial specialization of adapt, we make our own return type,
// separate from primer::result. Since we are throwing exceptions rather than
// returning error objects, the return can just be an int basically.
struct my_int {
  int value;
};

// Now we specialize adapt within the primer namespace
// In order to make it as simple as possible, we implement it by translating the
// `my_int + exception` return signal to primer::result, and adapt that version
// using PRIMER_ADAPT.
//
// Because PRIMER_ADAPT takes place at compile-time, this is all transparent to
// the optimizer and all of this can be inlined.
//
// Of course, you could also implement it without reference to `primer::result`
// at all, but you'd likely have to dig into the implementation details for the
// argument parsing off of the stack, or reimplement that yourself.
namespace primer {

template <typename... Args, my_int (*target_func)(lua_State *, Args...)>
class adapt<my_int (*)(lua_State *, Args...), target_func> {
  static primer::result adapt_target(lua_State * L, Args... args) {
    try {
      my_int r = target_func(L, std::forward<Args>(args)...);
      return r.value;
    } catch (raise_lua_error & e) { return primer::error{e.what()}; }
  }

public:
  // This is the `output`, that is, the function pointer which we produce.
  static int adapted(lua_State * L) { return (PRIMER_ADAPT(&adapt_target))(L); }
};

} // end namespace primer

//]

//[ primer_raise_lua_error_test
my_int
reverse_palindrome(lua_State * L, std::string p) {
  auto it = p.begin();
  auto it2 = p.end() - 1;

  while (it < it2) {
    if (*it != *it2) { throw raise_lua_error("not a palidrome"); }
    ++it;
    --it2;
  }

  if (it == it2) {
    std::string s{it, p.end()};
    s += std::string{p.begin() + 1, it + 1};
    primer::push(L, s);
  } else {
    std::string s{it, p.end()};
    s += std::string{p.begin(), it};
    primer::push(L, s);
  }
  return {1};
}

void
test_adapt_example() {
  lua_raii L;

  lua_CFunction f = PRIMER_ADAPT(&reverse_palindrome);

  lua_pushcfunction(L, f);
  lua_pushstring(L, "amanapanama");
  TEST_LUA_OK(L, lua_pcall(L, 1, 1, 0));
  TEST(lua_isstring(L, 1), "expected a string");
  TEST_EQ(lua_tostring(L, 1), std::string{"panamamanap"});

  lua_pushcfunction(L, f);
  lua_insert(L, 1);
  TEST_LUA_OK(L, lua_pcall(L, 1, 1, 0));
  TEST(lua_isstring(L, 1), "expected a string");
  TEST_EQ(lua_tostring(L, 1), std::string{"amanapanama"});

  lua_pushcfunction(L, f);
  lua_pushstring(L, "abfxxxx");
  TEST(LUA_OK != lua_pcall(L, 1, 1, 0), "expected failure");
}
//]

UNIT_TEST(test_adapt_example) { test_adapt_example(); }

int
main() {
  conf::log_conf();

  std::cout << "Core tests:" << std::endl;
  return test_registrar::run_tests();
}
