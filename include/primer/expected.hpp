//  (C) Copyright 2015 - 2016 Christopher Beck

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <primer/base.hpp>

PRIMER_ASSERT_FILESCOPE;

#include <primer/expected_fwd.hpp>
#include <primer/error.hpp>
#include <primer/support/asserts.hpp>

#include <string>
#include <type_traits>
#include <utility>

/***
 * A class for managing errors without throwing exceptions.
 * The idea is, expected<T> represents either an instance of T, or an error
 * message explaining why T could not be produced.
 * Based (loosely) on a talk by Andrei Alexandrescu
 * https://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Andrei-Alexandrescu-Systematic-Error-Handling-in-C
 */

#define PRIMER_BAD_ACCESS(X) PRIMER_ASSERT((X), "Bad access to primer::expected")

namespace primer {

/***
 * Expected class template
 */
template <typename T>
class expected {
  union {
    T ham_;
    error spam_;
  };
  bool have_ham_;


public:
  // Basic accessors & dereference semantics

  explicit operator bool() const noexcept { return have_ham_; }

  T & operator*() & {
    PRIMER_BAD_ACCESS(have_ham_);
    return ham_;
  }

  const T & operator*() const & {
    PRIMER_BAD_ACCESS(have_ham_);
    return ham_;
  }

  T && operator*() && {
    PRIMER_BAD_ACCESS(have_ham_);
    return std::move(ham_);
  }

  T * operator->() & {
    PRIMER_BAD_ACCESS(have_ham_);
    return &ham_;
  }

  const T * operator->() const & {
    PRIMER_BAD_ACCESS(have_ham_);
    return &ham_;
  }

  primer::error & err() & {
    PRIMER_BAD_ACCESS(!have_ham_);
    return spam_;
  }

  const primer::error & err() const & {
    PRIMER_BAD_ACCESS(!have_ham_);
    return spam_;
  }

  primer::error && err() && {
    PRIMER_BAD_ACCESS(!have_ham_);
    return std::move(spam_);
  }

  
private:
  // Internal manipulation: deinitialize, init_ham, init_spam

  void deinitialize() {
    if (have_ham_) {
      ham_.~T();
    } else {
      spam_.~error();
    }
  }

  // Both init_ham and init_spam assume that this is a fresh union or that
  // deiniitalize() was called!
  template <typename A>
  void init_ham(A &&  a) {
    new (&ham_) T(std::forward<A>(a));
    have_ham_ = true;
  }

  template <typename A>
  void init_spam(A && a) {
    new (&spam_) error(std::forward<A>(a));
    have_ham_ = false;
  }


  // Helpers for special member functions: Init from (potentially) another kind of expected.
  template <typename E>
  void init_from_const_ref(const E & e) {
    if (e) {
      this->init_ham(*e);
    } else {
      this->init_spam(e.err());
    }
  }

  template <typename E>
  void init_from_rvalue_ref(E && e) {
    if (e) {
      this->init_ham(std::move(*e));
    } else {
      this->init_spam(std::move(e.err()));
    }
  }

public:
  expected(const T & t) {
    this->init_ham(t);
  }

  expected(T && t) {
    this->init_ham(std::move(t));
  }

  // this is to make it so that we can return error structs from functions that
  // return expected
  expected(const primer::error & e) {
    this->init_spam(e);
  }

  expected(primer::error && e) {
    this->init_spam(std::move(e));
  }

  expected() {
    this->init_spam(primer::error{std::string{"uninit"}});
  }

  ~expected() {
    this->deinitialize();
  }

  // Copy ctor
  expected(const expected & e) {
    this->init_from_const_ref(e);
  }

  // When incoming expected has a different type, we allow conversion if U is
  // convertible to T.
  // Defer to copy assignment operator.
  template <typename U>
  expected(const expected<U> & u)
    : expected() // default ctor first
  {
    *this = u;
  }

  /***
   * Same thing with move constructors.
   */
  expected(expected && e) {
    this->init_from_rvalue_ref(std::move(e));
  }

  template <typename U>
  expected(expected<U> && u)
    : expected()
  {
    *this = u;
  }

  /***
   * Copy assignment operator
   *
   * We allow constructing expected<T> from expected<U> if U is convertible to
   * T.
   */
  expected & operator=(const expected & e) {
    this->deinitialize();
    this->init_from_const_ref(e);
    return *this;
  }

  template <typename U>
  expected & operator=(const expected<U> & u) {
    this->deinitialize();
    this->init_from_const_ref(u);
    return *this;
  }

  /***
   * Move assignment operator
   *
   * Same thing
   */
  expected & operator=(expected && e) {
    this->deinitialize();
    this->init_from_rvalue_ref(std::move(e));
    return *this;
  }

  template <typename U>
  expected & operator=(expected<U> && u) {
    this->deinitialize();
    this->init_from_rvalue_ref(std::move(u));
    return *this;
  }

  /***
   * Succinct access to error string
   */
  const std::string & err_str() const { return this->err().str(); }
  const char * err_c_str() const { return this->err_str().c_str(); }
};

/***
 * Allow references. This works by providing a new interface over `expected<T*>`.
 */

template <typename T>
class expected<T&> {
  expected<T*> internal_;

public:

  explicit operator bool() const noexcept { return static_cast<bool>(internal_); }

  T & operator*() & { return **internal_; }
  const T & operator*() const & { return **internal_; }
  T && operator*() && { return std::move(**internal_); }

  T * operator->() & { return *internal_; }
  const T * operator->() const & { return *internal_; }

  primer::error & err() & { return internal_.err(); }
  const primer::error & err() const & { return internal_.err(); }
  primer::error && err() && { return std::move(internal_.err()); }

  const std::string & err_str() const { return this->err().str(); }
  const char * err_c_str() const { return this->err_str().c_str(); }  

  // Ctors
  expected() = default;
  expected(const expected &) = default;
  expected(expected &&) = default;
  expected & operator = (const expected &) = default;
  expected & operator = (expected &&) = default;
  ~expected() = default;

  expected(T & t)
    : internal_(&t)
  {}

  expected(const primer::error & e)
    : internal_(e)
  {}

  expected(primer::error && e)
    : internal_(std::move(e))
  {}

  // Don't allow converting from other kinds of expected, since could get a dangling reference.
};

/***
 * Specialize the template for type "void".
 * Internally this is similar to "optional<error>", but has a matching interface as above, where operator bool
 * returning false signals the error.
 *
 * There is no operator * for this one.
 *
 * No union here.
 */
template <>
class expected<void> {
  bool no_error_;
  primer::error error_;

public:
  // this is to make it so that we can return error structs from functions that
  // return expected
  expected(const primer::error & e)
    : no_error_(false)
    , error_(e)
  {}

  expected(primer::error && e)
    : no_error_(false)
    , error_(std::move(e))
  {}

  expected()
    : no_error_(true)
    , error_("uninit")
  // : payload(util::error("uninit"))
  // ^ note the difference! default constructing this means "no error",
  //   for other expected types it means error
  {}

  // Conversion from other expected types
  // Note this ASSUMES that the other one has an error! Does not discard a value.
  template <typename U>
  expected(const expected<U> & u)
    : expected(u.err())
  {}

  template <typename U>
  expected(expected<U> && u)
    : expected(std::move(u.err()))
  {}

  // Copy and move operations
  expected(const expected &) = default;
  expected(expected &&) = default;
  expected & operator=(const expected &) = default;
  expected & operator=(expected &&) = default;

  // Public interface
  explicit operator bool() const noexcept { return no_error_; }

  const primer::error & err() const & {
    PRIMER_BAD_ACCESS(!no_error_);
    return error_;
  }
  primer::error && error() && {
    PRIMER_BAD_ACCESS(!no_error_);
    return std::move(error_);
  }

  const std::string & err_str() const {
    return this->err().str();
  }
  const char * err_c_str() const {
    return this->err_str().c_str();
  }
};

#undef PRIMER_BAD_ACCESS

} // end namespace primer
