// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_OPTIONAL_H
#define GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_OPTIONAL_H

#include "google/cloud/internal/throw_delegate.h"
#include <type_traits>
#include <utility>

namespace google {
namespace cloud {
inline namespace GOOGLE_CLOUD_CPP_NS {
/**
 * A poor's man version of std::optional<T>.
 *
 * This project needs to support C++11 and C++14, so std::optional<> is not
 * available.  We cannot use Abseil either, see #232 for the reasons.
 * So we implement a very minimal "optional" class that documents the intent
 * and we will remove it when possible.
 *
 * @tparam T the type of the optional value.
 *
 * @warning this is not a drop-in replacement for std::optional<>, it has at
 *   least the following defects:
 *   - it raises std::logic_error instead of std::bad_optional_access.
 *   - emplace() is a very simple version.
 *   - It does not have the full complement of assignment operators and
 *     constructors required by the standard.
 *   - It lacks comparison operators.
 *   - No nullopt_t.
 *   - No std::swap(), std::make_optional(), or std::hash().
 *
 * @warning Some of the member functions have a (commented-out) `constexpr`
 * qualifier. The spec requires them to be `constexpr` functions, but the spec
 * assumes C++14 (or newer) semantics for non-const constexpr member functions,
 * and we often compile with C++11 semantics.
 *
 * TODO(#687) - replace with absl::optional<> or std::optional<> when possible.
 */
template <typename T>
class optional {
 private:
  template <typename T1, typename U1>
  using AllowImplicit = typename std::is_convertible<U1&&, T1>;

 public:
  optional() : buffer_{}, has_value_(false) {}
  optional(optional const& rhs) : has_value_(rhs.has_value_) {
    if (has_value_) {
      new (reinterpret_cast<T*>(&buffer_)) T(*rhs);
    }
  }
  optional(optional&& rhs) : has_value_(rhs.has_value_) {
    if (has_value_) {
      new (reinterpret_cast<T*>(&buffer_)) T(std::move(*rhs));
    }
  }
  template <typename U = T,
            typename std::enable_if<
                !std::is_same<typename std::decay<U>::type, optional>::value &&
                    std::is_constructible<T, U&&>::value &&
                    AllowImplicit<T, U>::value,
                int>::type = 0>
  optional(U&& x) : has_value_(true) {
    new (reinterpret_cast<T*>(&buffer_)) T(std::forward<U>(x));
  }
  template <typename U = T,
            typename std::enable_if<
                !std::is_same<typename std::decay<U>::type, optional>::value &&
                    std::is_constructible<T, U&&>::value &&
                    !AllowImplicit<T, U>::value,
                int>::type = 0>
  explicit optional(U&& x) : has_value_(true) {
    new (reinterpret_cast<T*>(&buffer_)) T(std::forward<U>(x));
  }
  ~optional() { reset(); }

  optional& operator=(optional const& rhs) {
    // There may be shorter ways to express this, but this is fairly readable,
    // and should be reasonably efficient. Note that we must avoid destructing
    // the destination and/or default initializing it unless really needed.
    if (!has_value_) {
      if (!rhs.has_value_) {
        return *this;
      }
      new (reinterpret_cast<T*>(&buffer_)) T(*rhs);
      has_value_ = true;
      return *this;
    }
    if (!rhs.has_value_) {
      reset();
      return *this;
    }
    **this = *rhs;
    has_value_ = true;
    return *this;
  }

  optional& operator=(optional&& rhs) noexcept {
    // There may be shorter ways to express this, but this is fairly readable,
    // and should be reasonably efficient. Note that we must avoid destructing
    // the destination and/or default initializing it unless really needed.
    if (!has_value_) {
      if (!rhs.has_value_) {
        return *this;
      }
      new (reinterpret_cast<T*>(&buffer_)) T(std::move(*rhs));
      has_value_ = true;
      return *this;
    }
    if (!rhs.has_value_) {
      reset();
      return *this;
    }
    **this = std::move(*rhs);
    has_value_ = true;
    return *this;
  }

  // Disable this assignment if U==optional<T>. Well, really if U is a
  // cv-qualified version of optional<T>, so we need to apply std::decay<> to
  // it first.
  template <typename U = T>
  typename std::enable_if<
      !std::is_same<optional, typename std::decay<U>::type>::value,
      optional>::type&
  operator=(U&& rhs) {
    // There may be shorter ways to express this, but this is fairly readable,
    // and should be reasonably efficient. Note that we must avoid destructing
    // the destination and/or default initializing it unless really needed.
    if (!has_value_) {
      new (reinterpret_cast<T*>(&buffer_)) T(std::forward<U>(rhs));
      has_value_ = true;
      return *this;
    }
    **this = std::forward<U>(rhs);
    has_value_ = true;
    return *this;
  }

  constexpr T const* operator->() const {
    return reinterpret_cast<T const*>(&buffer_);
  }
  /*constexpr*/ T* operator->() { return reinterpret_cast<T*>(&buffer_); }
  constexpr T const& operator*() const& {
    return *reinterpret_cast<T const*>(&buffer_);
  }
  /*constexpr*/ T& operator*() & { return *reinterpret_cast<T*>(&buffer_); }
  // Kind of useless, but the spec requires it.
#if GOOGLE_CLOUD_CPP_HAVE_CONST_REF_REF
  /*constexpr*/ T const&& operator*() const&& {
    return std::move(*reinterpret_cast<T const*>(&buffer_));
  }
#endif  // GOOGLE_CLOUD_CPP_HAVE_CONST_REF_REF
  /*constexpr*/ T&& operator*() && {
    return std::move(*reinterpret_cast<T*>(&buffer_));
  }

  /*constexpr*/ T& value() & {
    check_access();
    return **this;
  }
  /*constexpr*/ T const& value() const& {
    check_access();
    return **this;
  }
  /*constexpr*/ T&& value() && {
    check_access();
    return std::move(**this);
  }
  // Kind of useless, but the spec requires it.
  /*constexpr*/ T const&& value() const&& {
    check_access();
    return **this;
  }

  template <typename U>
  constexpr T value_or(U&& default_value) const& {
    return bool(*this) ? **this
                       : static_cast<T>(std::forward<U>(default_value));
  }

  template <typename U>
  /*constexpr*/ T value_or(U&& default_value) && {
    return bool(*this) ? std::move(**this)
                       : static_cast<T>(std::forward<U>(default_value));
  }

  explicit operator bool() const { return has_value_; }
  bool has_value() const { return has_value_; }

  void reset() {
    if (has_value_) {
      reinterpret_cast<T*>(&buffer_)->~T();
      has_value_ = false;
    }
  }
  T& emplace(T&& value) {
    reset();
    new (reinterpret_cast<T*>(&buffer_)) T(std::move(value));
    has_value_ = true;
    return **this;
  }

 private:
  void check_access() const {
    if (has_value_) {
      return;
    }
    google::cloud::internal::ThrowLogicError("access unset optional");
  }

  // We use std::aligned_storage<T> because T may not have a default
  // constructor, if we used 'T' here we could not default initialize this class
  // either.
  using aligned_storage_t = std::aligned_storage<sizeof(T), alignof(T)>;
  typename aligned_storage_t::type buffer_;
  bool has_value_;
};

template <typename T>
optional<T> make_optional(T&& t) {
  return optional<T>(std::forward<T>(t));
}

template <typename T>
inline bool operator==(optional<T> const& lhs, optional<T> const& rhs) {
  if (lhs.has_value() != rhs.has_value()) {
    return false;
  }
  if (!lhs.has_value()) {
    return true;
  }
  return *lhs == *rhs;
}

template <typename T>
inline bool operator<(optional<T> const& lhs, optional<T> const& rhs) {
  if (lhs.has_value()) {
    if (!rhs.has_value()) {
      return false;
    }
    // Both have a value, compare them
    return *lhs < *rhs;
  }
  // If both do not have a value, then they are equal, so this returns false.
  // If rhs has a value then it compares larger than lhs because lhs does
  // not have a value.
  return rhs.has_value();
}

template <typename T>
inline bool operator!=(optional<T> const& lhs, optional<T> const& rhs) {
  return std::rel_ops::operator!=(lhs, rhs);
}

template <typename T>
inline bool operator>(optional<T> const& lhs, optional<T> const& rhs) {
  return std::rel_ops::operator>(lhs, rhs);
}

template <typename T>
inline bool operator>=(optional<T> const& lhs, optional<T> const& rhs) {
  return std::rel_ops::operator>=(lhs, rhs);
}

template <typename T>
inline bool operator<=(optional<T> const& lhs, optional<T> const& rhs) {
  return std::rel_ops::operator<=(lhs, rhs);
}

}  // namespace GOOGLE_CLOUD_CPP_NS
}  // namespace cloud
}  // namespace google

#endif  // GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_OPTIONAL_H
