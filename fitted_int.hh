/* Fitted fixed-width integer types
 *
 * Author: Ryan Gambord <Ryan.Gambord@oregonstate.edu>
 * Date: July 26 2023
 */
#pragma once
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>

namespace util
{
namespace fitted_int
{

template <auto X, class T = void, class... VTs>
struct fit_to_list {
  using type = std::conditional_t<
      ((std::conditional_t<std::is_signed_v<T>, std::intmax_t, std::uintmax_t>)
               X >= std::numeric_limits<T>::min() &&
       (std::conditional_t<std::is_signed_v<T>, std::intmax_t, std::uintmax_t>)
               X <= std::numeric_limits<T>::max()),
      T, typename fit_to_list<X, VTs...>::type>;
};

template <auto X>
struct fit_to_list<X, void> {
  using type = void;
};

template <auto X, class T = void, class... VTs>
using fit_to_list_t = typename fit_to_list<X, T, VTs...>::type;

#define fit_base(base)                                                         \
  template <auto X>                                                            \
  using base##X_t =                                                            \
      fit_to_list_t<X, std::base##8_t, std::base##16_t, std::base##32_t,       \
                    std::base##64_t, std::intmax_t>

fit_base(int);
fit_base(int_fast);
fit_base(int_least);
fit_base(uint);
fit_base(uint_fast);
fit_base(uint_least);

template <std::size_t N>
class uint_exactX_t
{
  static_assert(N <= std::numeric_limits<std::uintmax_t>::digits);
  static_assert(N > 0);

public:
  /* clang-format off */
  using value_type = 
    typename std::conditional<N <= 8, std::uint8_t,
    typename std::conditional<N <= 16, std::uint16_t,
    typename std::conditional<N <= 32, std::uint32_t,
    typename std::conditional<N <= 64, std::uint64_t,
      std::uintmax_t>::type>::type>::type>::type;
  /* clang-format on */
private:
  value_type n;
  constexpr static value_type mask = (~(value_type)0) >> (std::numeric_limits<value_type>::digits - N);

public:
  template <class T2>
  constexpr uint_exactX_t(T2 val) : n{((value_type)val) & mask}
  {
  }

  constexpr uint_exactX_t() : n{0} {}
  constexpr uint_exactX_t(uint_exactX_t const &other) : n{other.n} {}
  constexpr uint_exactX_t(uint_exactX_t &&other) : n{other.n} {}

  template <class T2>
  constexpr operator T2() const
  {
    return n;
  }

#define overload_op(op)                                                        \
  template <class T2>                                                          \
  constexpr auto &operator op##=(T2 rhs) const                                 \
  {                                                                            \
    n = (n op rhs)&mask;                                                       \
    return *this;                                                              \
  }                                                                            \
  template <class T2>                                                          \
  friend constexpr auto operator op(uint_exactX_t const &lhs, T2 const &rhs)   \
  {                                                                            \
    return lhs.n op rhs;                                                       \
  }                                                                            \
  template <class T2>                                                          \
  friend constexpr auto operator op(T2 const &lhs, uint_exactX_t const &rhs)   \
  {                                                                            \
    return lhs op rhs.n;                                                       \
  }

  overload_op(+);
  overload_op(-);
  overload_op(*);
  overload_op(/);
  overload_op(%);
  overload_op(^);
  overload_op(&);
  overload_op(|);
  overload_op(<<);
  overload_op(>>);
#undef overload_op

  template <class T2>
  constexpr uint_exactX_t &operator=(T2 rhs)
  {
    n = ((value_type)rhs) & mask;
    return *this;
  }

  constexpr uint_exactX_t operator~() const { return (uint_exactX_t){~n}; }
  constexpr bool operator!() const { return !n; }
  constexpr operator bool() const { return n; }

#define overload_op(op)                                                        \
  template <class T2>                                                          \
  friend constexpr bool operator op(uint_exactX_t const &lhs, T2 const &rhs)   \
  {                                                                            \
    return lhs.n op rhs;                                                       \
  }                                                                            \
  template <class T2>                                                          \
  friend constexpr bool operator op(T2 const &lhs, uint_exactX_t const &rhs)   \
  {                                                                            \
    return lhs op rhs.n;                                                       \
  }

  overload_op(==);
  overload_op(!=);
  overload_op(<=);
  overload_op(>=);
  overload_op(&&);
  overload_op(||);
#undef overload_op
#define overload_op(op)                                                        \
  constexpr uint_exactX_t &operator op()                                       \
  {                                                                            \
    op n;                                                                      \
    n &= mask;                                                                 \
    return *this;                                                              \
  }                                                                            \
  constexpr uint_exactX_t operator op(int)                                     \
  {                                                                            \
    auto ret = *this;                                                          \
    op n;                                                                      \
    n &= mask;                                                                 \
    return ret;                                                                \
  }

  overload_op(++);
  overload_op(--);

  template <class CharT, class Traits>
  friend std::basic_ostream<CharT, Traits> &
  operator<<(std::basic_ostream<CharT, Traits> &os, uint_exactX_t const &rhs)
  {
    return (os.operator<<(rhs.n));
  }

  template <class CharT, class Traits>
  friend std::basic_istream<CharT, Traits> &
  operator>>(std::basic_istream<CharT, Traits> &is, uint_exactX_t &rhs)
  {
    return (is.operator>>(rhs.n));
  }
};

} // namespace fitted_int
} // namespace util
