#include <primer/std.hpp>

#include <primer/push.hpp>
#include <primer/read.hpp>

#include "test_harness.hpp"
#include <iostream>
#include <string>

#include <array>
#include <map>
#include <vector>

void test_vector_push() {
  lua_raii L;

  std::vector<int> vec {{5, 6, 7, 8}};

  primer::push(L, vec);
  CHECK_STACK(L, 1);
  test_top_type(L, LUA_TTABLE, __LINE__);

  std::size_t n = lua_rawlen(L, -1);
  TEST_EQ(n, vec.size());

  lua_rawgeti(L, 1, 1);
  TEST(lua_isinteger(L, -1), "not an integer");
  TEST_EQ(lua_tointeger(L, -1), 5);
  lua_pop(L, 1);

  lua_rawgeti(L, 1, 2);
  TEST(lua_isinteger(L, -1), "not an integer");
  TEST_EQ(lua_tointeger(L, -1), 6);
  lua_pop(L, 1);

  lua_rawgeti(L, 1, 3);
  TEST(lua_isinteger(L, -1), "not an integer");
  TEST_EQ(lua_tointeger(L, -1), 7);
  lua_pop(L, 1);

  lua_rawgeti(L, 1, 4);
  TEST(lua_isinteger(L, -1), "not an integer");
  TEST_EQ(lua_tointeger(L, -1), 8);
  lua_pop(L, 1);

  lua_rawgeti(L, 1, 5);
  TEST(lua_isnil(L, -1), "expected nil");
  lua_pop(L, 1);
}

void test_array_push() {
  lua_raii L;

  std::array<int, 4> arr {{5, 6, 7, 8}};

  primer::push(L, arr);
  CHECK_STACK(L, 1);
  test_top_type(L, LUA_TTABLE, __LINE__);

  std::size_t n = lua_rawlen(L, -1);
  TEST_EQ(n, arr.size());

  lua_rawgeti(L, 1, 1);
  TEST(lua_isinteger(L, -1), "not an integer");
  TEST_EQ(lua_tointeger(L, -1), 5);
  lua_pop(L, 1);

  lua_rawgeti(L, 1, 2);
  TEST(lua_isinteger(L, -1), "not an integer");
  TEST_EQ(lua_tointeger(L, -1), 6);
  lua_pop(L, 1);

  lua_rawgeti(L, 1, 3);
  TEST(lua_isinteger(L, -1), "not an integer");
  TEST_EQ(lua_tointeger(L, -1), 7);
  lua_pop(L, 1);

  lua_rawgeti(L, 1, 4);
  TEST(lua_isinteger(L, -1), "not an integer");
  TEST_EQ(lua_tointeger(L, -1), 8);
  lua_pop(L, 1);

  lua_rawgeti(L, 1, 5);
  TEST(lua_isnil(L, -1), "expected nil");
  lua_pop(L, 1);
}

void test_map_push() {
  {
    std::map<std::string, std::string> my_map{{"a", "1"}, {"b", "2"}, {"c", "3"}};

    lua_raii L;

    primer::push(L, my_map);
    CHECK_STACK(L, 1);
    test_top_type(L, LUA_TTABLE, __LINE__);

    lua_pushnil(L);
    int counter = 0;
    while (lua_next(L, 1)) {
      ++counter;
      TEST(lua_isstring(L, 2), "expected a string key");
      std::string key = lua_tostring(L, 2);
      TEST(lua_isstring(L, 3), "expected a string val");
      std::string val = lua_tostring(L, 3);

      TEST_EQ(val, my_map[key]);
      my_map.erase(key);

      lua_pop(L, 1);
    }
    TEST_EQ(my_map.size(), 0);
    TEST_EQ(counter, 3);
  }

  {
    std::map<int, int> my_map{{'a', 1}, {'b', 2}, {'c', 3}};

    lua_raii L;

    primer::push(L, my_map);
    CHECK_STACK(L, 1);
    test_top_type(L, LUA_TTABLE, __LINE__);

    lua_pushnil(L);
    int counter = 0;
    while (lua_next(L, 1)) {
      ++counter;
      TEST(lua_isinteger(L, 2), "expected an int key");
      int key = lua_tointeger(L, 2);
      TEST(lua_isinteger(L, 3), "expected an int val");
      int val = lua_tointeger(L, 3);

      TEST_EQ(val, my_map[key]);
      my_map.erase(key);

      lua_pop(L, 1);
    }
    TEST_EQ(my_map.size(), 0);
    TEST_EQ(counter, 3);
  }
}

int main() {
  conf::log_conf();

  std::cout << "Std container tests:" << std::endl;
  test_harness tests{
    {"test vector push", &test_vector_push},
    {"test array push", &test_array_push},
    {"test map push", &test_map_push},
  };
  int num_fails = tests.run();
  std::cout << "\n";
  if (num_fails) {
    std::cout << num_fails << " tests failed!" << std::endl;
    return 1;
  } else {
    std::cout << "All tests passed!" << std::endl;
    return 0;
  }
}
