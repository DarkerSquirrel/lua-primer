//  (C) Copyright 2015 - 2018 Christopher Beck

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef> // for std::size_t
#include <primer/support/assert_filescope.hpp>

PRIMER_ASSERT_FILESCOPE;

#include <primer/conf.hpp>
#include <primer/version.hpp>

#include <primer/primer_fwd.hpp>

// Forward declare some lua types
struct lua_State;
typedef int (*lua_CFunction)(lua_State *);

// Force PRIMER_NO_MEMORY_FAILURE if lua is compiled as C
#if !defined(PRIMER_LUA_AS_CPP) && !defined(PRIMER_NO_MEMORY_FAILURE)
#define PRIMER_NO_MEMORY_FAILURE
#endif

// Enable static assertions
#ifndef PRIMER_NO_STATIC_ASSERTS
#define PRIMER_STATIC_ASSERT(C, M) static_assert(C, M)
#else
#define PRIMER_STATIC_ASSERT(C, M) static_assert(true, "")
#endif

// Enable exception handling
#ifndef PRIMER_NO_EXCEPTIONS
#define PRIMER_TRY try
#define PRIMER_CATCH(X) catch (X)
#define PRIMER_RETHROW throw
#else
#define PRIMER_TRY if (1)
#define PRIMER_CATCH(X) if (0)
#define PRIMER_RETHROW static_assert(true, "")
#endif

// Implement PRIMER_NO_MEMORY_FAILURE for std::bad_alloc handling
#ifdef PRIMER_NO_MEMORY_FAILURE
#define PRIMER_TRY_BAD_ALLOC PRIMER_TRY
#define PRIMER_CATCH_BAD_ALLOC PRIMER_CATCH(std::bad_alloc &)
#else
#define PRIMER_TRY_BAD_ALLOC if (1)
#define PRIMER_CATCH_BAD_ALLOC if (0)
#endif
