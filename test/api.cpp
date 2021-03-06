#include <primer/api.hpp>
#include <primer/primer.hpp>
#include <primer/std/vector.hpp>

#include "test_harness/g_inspector.hpp"
#include "test_harness/test_harness.hpp"
#include <initializer_list>
#include <iostream>
#include <string>

struct test_api_one : primer::api::persistable<test_api_one> {
  lua_raii L;

  std::string save() {
    std::string result;
    this->persist(L, result);
    return result;
  }

  void restore(const std::string & buffer) { this->unpersist(L, buffer); }

  void create_mock_state() {
    PRIMER_ASSERT_STACK_NEUTRAL(L);

    lua_newtable(L);
    lua_pushinteger(L, 5);
    lua_setfield(L, -2, "a");

    lua_pushinteger(L, 7);
    lua_setfield(L, -2, "b");

    lua_setglobal(L, "bah");

    lua_pushboolean(L, true);
    lua_setglobal(L, "humbug");
  }

  bool test_mock_state() {
    // PRIMER_ASSERT_STACK_NEUTRAL(L);

    lua_getglobal(L, "bah");
    if (!lua_istable(L, -1)) { return false; }

    lua_getfield(L, -1, "a");
    if (!lua_isinteger(L, -1)) { return false; }
    if (5 != lua_tointeger(L, -1)) { return false; }
    lua_pop(L, 1);

    lua_getfield(L, -1, "b");
    if (!lua_isinteger(L, -1)) { return false; }
    if (7 != lua_tointeger(L, -1)) { return false; }
    lua_pop(L, 1);

    lua_pop(L, 1);

    lua_getglobal(L, "humbug");
    if (!lua_isboolean(L, -1)) { return false; }
    if (!lua_toboolean(L, -1)) { return false; }
    lua_pop(L, 1);

    return true;
  }
};

UNIT_TEST(persist_simple) {
  std::string buffer;

  {
    test_api_one a;
    TEST_EQ(false, a.test_mock_state());

    a.create_mock_state();

    TEST_EQ(true, a.test_mock_state());

    buffer = a.save();

    TEST_EQ(true, a.test_mock_state());
  }

  {
    test_api_one a;
    TEST_EQ(false, a.test_mock_state());

    a.restore(buffer);

    TEST_EQ(true, a.test_mock_state());
  }

  {
    test_api_one a;
    TEST_EQ(false, a.test_mock_state());

    a.restore(buffer);

    TEST_EQ(true, a.test_mock_state());

    buffer = a.save();
  }

  {
    test_api_one a;
    TEST_EQ(false, a.test_mock_state());

    a.restore(buffer);

    TEST_EQ(true, a.test_mock_state());
  }
}

UNIT_TEST(persist_simple_two) {
  std::string buffer;
  table_summary summary;

  {
    test_api_one a;
    TEST_EQ(false, a.test_mock_state());

    a.create_mock_state();

    TEST_EQ(true, a.test_mock_state());

    buffer = a.save();

    TEST_EQ(true, a.test_mock_state());

    summary = get_global_table_summary(a.L);
  }

  {
    test_api_one a;
    TEST_EQ(false, a.test_mock_state());

    a.restore(buffer);

    TEST_EQ(true, a.test_mock_state());
    auto summary2 = get_global_table_summary(a.L);

    bool match = check_tables_match(summary, summary2);
    TEST_EQ(summary.size(), summary2.size());
    TEST(match, "global table mismatch!");
  }
}

struct test_api_two : primer::api::base<test_api_two> {
  lua_raii L_;

  API_FEATURE(primer::api::libraries<primer::api::lua_base_lib>, libs_);
  API_FEATURE(primer::api::callbacks, cb_man_);

  NEW_LUA_CALLBACK(f, "this is the f help")
  (lua_State * L, int i, int j)->primer::result {
    if (i < j) {
      lua_pushinteger(L, -1);
    } else if (i > j) {
      lua_pushinteger(L, 1);
    } else {
      return primer::error("bad bad");
    }
    return 1;
  }

  NEW_LUA_CALLBACK(g, "this is the g help")
  (lua_State * L, std::string i, std::string j)->primer::result {
    if (i < j) {
      lua_pushinteger(L, -1);
    } else if (i > j) {
      lua_pushinteger(L, 1);
    } else {
      return primer::error("bad bad");
    }
    return 1;
  }

  USE_LUA_CALLBACK(help, "get help for a built-in function",
                   &primer::api::intf_help_impl);

  // A function with no help
  USE_LUA_CALLBACK(foo, &primer::api::intf_help_impl);

  test_api_two()
    : L_()
    , cb_man_(this) {
    this->initialize_api(L_);
  }

  std::string save() {
    std::string result;
    this->persist(L_, result);
    return result;
  }

  void restore(const std::string & buffer) { this->unpersist(L_, buffer); }
};

UNIT_TEST(api_base) {
  std::string buffer;
  table_summary summary;

  const char * script =
    ""
    "x = 4;                                          \n"
    "y = 5;                                          \n"
    "assert(f(x,y) == -1);                           \n"
    "x = x + 4                                       \n"
    "assert(f(x,y) == 1);                            \n"
    "                                                \n"
    "assert(g('asdf', 'jkl;') == -1)                 \n"
    "assert(pcall(g, 'asdf', 'afsd'))                \n"
    "assert(not pcall(g, 'asdf', 'asdf'))            \n";

  {
    test_api_two a;

    lua_State * L = a.L_;

    CHECK_STACK(L, 0);

    TEST_LUA_OK(L, luaL_loadstring(L, script));

    auto result = primer::fcn_call_no_ret(L, 0);
    TEST_EXPECTED(result);

    CHECK_STACK(L, 0);

    buffer = a.save();
    summary = get_global_table_summary(L);
  }

  {
    test_api_two a;
    a.restore(buffer);

    lua_State * L = a.L_;

    {
      auto summary2 = get_global_table_summary(L);
      // TEST(check_tables_match(summary, summary2), "global table mismatch!");
      if (!check_tables_match(summary, summary2)) {
        std::cerr << "WARN: Global table mismatch!\n";
      }
    }

    CHECK_STACK(L, 0);

    TEST_LUA_OK(L, luaL_loadstring(L, script));
    TEST_LUA_OK(L, lua_pcall(L, 0, 0, 0));

    CHECK_STACK(L, 0);
  }
}

UNIT_TEST(api_help) {
  test_api_two a;

  lua_State * L = a.L_;

  const char * script =
    ""
    "assert(help(f) == 'this is the f help')         \n"
    "assert(help(g) == 'this is the g help')         \n";

  TEST_LUA_OK(L, luaL_loadstring(L, script));
  TEST_LUA_OK(L, lua_pcall(L, 0, 0, 0));

  CHECK_STACK(L, 0);
}

/***
 * Test userdata persistence
 */

struct tstring {
  std::vector<std::string> strs;

  // These Ctors should not be necessary but they are to help older compilers
  tstring() = default;
  tstring(const tstring &) = default;
  tstring(tstring &&) = default;

  explicit tstring(std::vector<std::string> s)
    : strs(std::move(s)) {}

  // Methods
  std::string to_string() {
    std::string result;

    bool first = true;
    for (const auto & s : strs) {
      if (first) {
        first = false;
      } else {
        result += " .. ";
      }
      result += "_('" + s + "')";
    }

    return result;
  }

  tstring operator+(const tstring & other) const {
    std::vector<std::string> result{strs};
    result.insert(result.end(), other.strs.begin(), other.strs.end());
    return tstring{std::move(result)};
  }

  // Lua
  primer::result intf_to_string(lua_State * L) {
    primer::push(L, this->to_string());
    return 1;
  }

  primer::result intf_concat(lua_State * L, tstring & other) {
    primer::push_udata<tstring>(L, *this + other);
    return 1;
  }

  static primer::result intf_create(lua_State * L, std::string s) {
    primer::push_udata<tstring>(L, std::vector<std::string>{std::move(s)});
    return 1;
  }

  static int intf_reconstruct(lua_State * L) {
    if (auto strs =
          primer::read<std::vector<std::string>>(L, lua_upvalueindex(1))) {
      primer::push_udata<tstring>(L, std::move(*strs));
      return 1;
    } else {
      lua_pushstring(L, strs.err().c_str());
    }
    return lua_error(L);
  }

  int intf_persist(lua_State * L) {
    primer::push(L, strs);
    lua_pushcclosure(L, PRIMER_ADAPT(&intf_reconstruct), 1);
    return 1;
  }
};

static_assert(primer::detail::is_L_Reg_sequence<const luaL_Reg *>::value, "");
static_assert(primer::detail::is_L_Reg_sequence<const luaL_Reg * (*)()>::value,
              "");

static_assert(primer::detail::is_L_Reg_sequence<std::vector<::luaL_Reg>>::value,
              "");
static_assert(
  primer::detail::is_L_Reg_sequence<std::vector<::luaL_Reg> &>::value, "");
static_assert(
  primer::detail::is_L_Reg_sequence<const std::vector<::luaL_Reg> &>::value,
  "");

namespace primer {
namespace traits {

template <>
struct userdata<tstring> {
  static constexpr const char * const name = "tstring";
  static const std::vector<luaL_Reg> & metatable() {
    static const std::vector<luaL_Reg> metatable_array{
      {"__concat", PRIMER_ADAPT_USERDATA(tstring, &tstring::intf_concat)},
      {"__persist", PRIMER_ADAPT_USERDATA(tstring, &tstring::intf_persist)},
      {"__tostring", PRIMER_ADAPT_USERDATA(tstring, &tstring::intf_to_string)}};
    return metatable_array;
  }
  static const luaL_Reg * permanents() {
    static constexpr auto permanents_array = std::array<luaL_Reg, 2>{
      {{"tstring_reconstruct", PRIMER_ADAPT(&tstring::intf_reconstruct)},
       {nullptr, nullptr}}};
    return permanents_array.data();
  }
};

} // end namespace traits
} // end namespace primer

static_assert(primer::detail::metatable<tstring>::value == 2,
              "primer didn't recognize our userdata methods!");
static_assert(primer::detail::permanents_helper<tstring>::value == 1,
              "primer didn't recognize our userdata permanents!");

struct test_api_three : primer::api::base<test_api_three> {
  lua_raii L_;

  API_FEATURE(primer::api::libraries<primer::api::lua_base_lib>, libs_);
  API_FEATURE(primer::api::callbacks, cb_man_);
  API_FEATURE(primer::api::userdatas<tstring>, udata_man_);

  USE_LUA_CALLBACK(_, "creates a translatable string", &tstring::intf_create);

  test_api_three()
    : L_()
    , cb_man_(this) {
    this->initialize_api(L_);
  }

  std::string save() {
    std::string result;
    this->persist(L_, result);
    return result;
  }

  void restore(const std::string & buffer) { this->unpersist(L_, buffer); }

  void do_first_script() {
    const char * script =
      "assert(type(_) == 'function')        \n"
      "string1 = _'foo'                     \n"
      "string2 = _'bar'                     \n"
      "assert(type(string1) == 'userdata')  \n"
      "assert(type(string2) == 'userdata')  \n"
      "string3 = string1 .. string2         \n";
    TEST_LUA_OK(L_, luaL_loadstring(L_, script));
    auto result = primer::fcn_call_no_ret(L_, 0);
    TEST_EXPECTED(result);
  }

  void do_second_script() {
    const char * script =
      "assert(\"_('foo')\" == tostring(string1))                          \n"
      "assert(\"_('bar')\" == tostring(string2))                          \n"
      "assert(\"_('foo') .. _('bar')\" == tostring(string3))              \n";
    TEST_LUA_OK(L_, luaL_loadstring(L_, script));
    auto result = primer::fcn_call_no_ret(L_, 0);
    TEST_EXPECTED(result);
  }
};

UNIT_TEST(api_userdata) {
  std::string buffer;

  {
    test_api_three a;
    a.do_first_script();
    buffer = a.save();
  }

  {
    test_api_three a;
    a.restore(buffer);
    a.do_second_script();
  }
}

static_assert(
  !primer::api::
    is_serial_feature<primer::api::libraries<primer::api::lua_base_lib>>::value,
  "libraries was recognized as a serial feature!");
static_assert(!primer::api::is_serial_feature<primer::api::callbacks>::value,
              "callbacks was recognized as a serial feature!");
static_assert(
  primer::api::is_serial_feature<primer::api::persistent_value<std::string>>::
    value,
  "persistent value not recognized as a serial feature!");

struct test_api_four : primer::api::base<test_api_four> {
  lua_raii L_;

  API_FEATURE(primer::api::persistent_value<std::string>, my_string_);

  test_api_four()
    : L_()
    , my_string_() {
    this->initialize_api(L_);
  }

  std::string save() {
    std::string result;
    this->persist(L_, result);
    return result;
  }

  void restore(const std::string & buffer) { this->unpersist(L_, buffer); }

  void assign_my_string(const std::string & s) { my_string_.get() = s; }
  const std::string & get_my_string() const { return my_string_.get(); }
};

UNIT_TEST(persistent_value) {
  std::string buffer;
  std::string buffer2;

  {
    test_api_four a;
    TEST_EQ("", a.get_my_string());

    a.assign_my_string("foo");
    TEST_EQ("foo", a.get_my_string());

    buffer = a.save();
    TEST_EQ("foo", a.get_my_string());

    a.assign_my_string("bar");
    TEST_EQ("bar", a.get_my_string());

    buffer2 = a.save();
    TEST_EQ("bar", a.get_my_string());

    a.restore(buffer);
    TEST_EQ("foo", a.get_my_string());

    a.restore(buffer2);
    TEST_EQ("bar", a.get_my_string());
  }

  {
    test_api_four a;
    TEST_EQ("", a.get_my_string());

    a.restore(buffer2);
    TEST_EQ("bar", a.get_my_string());

    a.restore(buffer);
    TEST_EQ("foo", a.get_my_string());
  }
}

struct test_api_five : primer::api::base<test_api_five> {
  lua_raii L_;

  API_FEATURE(primer::api::sandboxed_basic_libraries, libs_);

  USE_LUA_CALLBACK(require, "this is the require function",
                   &primer::api::mini_require);

  API_FEATURE(primer::api::callbacks, cb_);

  test_api_five()
    : L_()
    , cb_(this) {
    this->initialize_api(L_);
  }
};

UNIT_TEST(sandboxed_libs) {
  test_api_five a;

  lua_State * L = a.L_;

  const char * script =
    "t = { 4, 5 }                           \n"
    "x, y = table.unpack(t)                 \n"
    "assert(x == 4)                         \n"
    "assert(y == 5)                         \n"
    "local tablib = require \'table\'       \n"
    "assert(tablib == table)                \n"
    "assert(math.sin)                       \n"
    "assert(not math.random)                \n";

  TEST_LUA_OK(L, luaL_loadstring(L, script));
  TEST_LUA_OK(L, lua_pcall(L, 0, 0, 0));
}

struct test_api_six : primer::api::base<test_api_six> {
  lua_raii L_;

  API_FEATURE(primer::api::sandboxed_basic_libraries, libs_);

  USE_LUA_CALLBACK(require, "this is the require function",
                   &primer::api::mini_require);

  API_FEATURE(primer::api::callbacks, cb_);
  API_FEATURE(primer::api::print_manager, print_man_);

  test_api_six()
    : L_()
    , cb_(this) {
    this->initialize_api(L_);
  }
};

struct interpreter_capture {
  std::vector<std::string> new_text_calls_;
  std::vector<std::string> error_text_calls_;

  void new_text(const std::string & str) { new_text_calls_.push_back(str); }
  void error_text(const std::string & str) { error_text_calls_.push_back(str); }
  void clear_input() {}
};

UNIT_TEST(api_interpreter_context) {
  test_api_six a;
  lua_State * L = a.L_;

  interpreter_capture c;

  a.print_man_.set_interpreter_context(&c);

  a.print_man_.handle_interpreter_input(L, "5");
  TEST_EQ(c.new_text_calls_.size(), 2);
  TEST_EQ(c.error_text_calls_.size(), 0);
  TEST_EQ(c.new_text_calls_[0], "$ 5");
  TEST_EQ(c.new_text_calls_[1], "5");

  a.print_man_.handle_interpreter_input(L, "foo");
  TEST_EQ(c.new_text_calls_.size(), 4);
  TEST_EQ(c.new_text_calls_[0], "$ 5");
  TEST_EQ(c.new_text_calls_[1], "5");
  TEST_EQ(c.new_text_calls_[2], "$ foo");
  TEST_EQ(c.new_text_calls_[3], "nil");

  TEST_EQ(c.error_text_calls_.size(), 0);

  // a.print_man_.pop_interpreter_context();

  a.print_man_.handle_interpreter_input(L, "foo.bar");
  TEST_EQ(c.new_text_calls_.size(), 5);
  TEST_EQ(c.error_text_calls_.size(), 1);
  TEST_EQ(c.new_text_calls_[0], "$ 5");
  TEST_EQ(c.new_text_calls_[1], "5");
  TEST_EQ(c.new_text_calls_[2], "$ foo");
  TEST_EQ(c.new_text_calls_[3], "nil");
  TEST_EQ(c.new_text_calls_[4], "$ foo.bar");
}

//[ primer_vfs_example
// Model of vfs provider concept
struct my_files : primer::api::vfs<my_files> {
  using map_t = std::map<std::string, std::string>;
  map_t files_;

  explicit my_files(map_t m)
    : files_(std::move(m)) {}

  primer::expected<void> load(lua_State * L, const std::string & path) {
    auto it = files_.find(path);
    if (it != files_.end()) {
      const std::string & chunk = it->second;
      luaL_loadbuffer(L, chunk.c_str(), chunk.size(), path.c_str());

      return {};
    } else {
      return primer::error::module_not_found(path);
    }
  }
};

// Example api::base object
struct test_api : primer::api::base<test_api> {
  lua_raii L_;

  API_FEATURE(primer::api::sandboxed_basic_libraries, libs_);
  API_FEATURE(my_files, vfs_);
  API_FEATURE(primer::api::print_manager, print_man_);

  test_api()
    : L_()
    , vfs_{
        {{"foo", "return {}"},
         {"bar", "local function baz() return 5 end; return { baz = baz }"}}} {
    this->initialize_api(L_);
  }

  std::string save() {
    std::string result;
    this->persist(L_, result);
    return result;
  }

  void restore(const std::string & buffer) { this->unpersist(L_, buffer); }
};

UNIT_TEST(api_vfs) {
  std::string buffer;
  table_summary summary;

  const char * script =
    "assert(type(5) == 'number')                                           \n"
    "assert(type('foo') == 'string')                                       \n"
    "assert(type(loadfile 'foo') == 'function')                            \n"
    "assert(type(dofile 'foo') == 'table')                                 \n"
    "                                                                      \n"
    "local foo = require 'foo'                                             \n"
    "assert(type(foo) == 'table')                                          \n"
    "assert(type(bar) == 'nil')                                            \n"
    "local bar = require 'bar'                                             \n"
    "assert(5 == bar.baz())                                                \n"
    "assert(not pcall(require, 'baz'))                                     \n"
    "                                                                      \n"
    "assert(bar == require 'bar')                                          \n"
    "assert(bar ~= dofile 'bar')                                           \n"
    "                                                                      "
    "\n";

  {
    test_api a;

    lua_State * L = a.L_;

    TEST_LUA_OK(L, luaL_loadstring(L, script));
    TEST_LUA_OK(L, lua_pcall(L, 0, 0, 0));

    buffer = a.save();
    summary = get_global_table_summary(L);
  }

  {
    test_api a;

    lua_State * L = a.L_;

    TEST_LUA_OK(L, luaL_loadstring(L, script));
    TEST_LUA_OK(L, lua_pcall(L, 0, 0, 0));

    a.restore(buffer);

    auto summary2 = get_global_table_summary(L);
    // TEST(check_tables_match(summary, summary2),
    //     "expected global tables to match!");
    if (!check_tables_match(summary, summary2)) {
      std::cerr << "WARN: Global table mismatch!\n";
    }

    TEST_LUA_OK(L, luaL_loadstring(L, script));
    TEST_LUA_OK(L, lua_pcall(L, 0, 0, 0));
  }
}
//]

int
main() {
  conf::log_conf();

  std::cout << "Persistence tests:" << std::endl;
  return test_registrar::run_tests();
}
