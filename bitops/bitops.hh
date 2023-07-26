/* Portable <bit> implementation
 *
 * Tested with gcc 13.1 -O3 -std=c++11
 * Output assembly is identical between portable implementation and library methods
 *
 * Author: Ryan Gambord <Ryan.Gambord@oregonstate.edu>
 * Date: Jul 26 2023
 */
#pragma once

#if __cpp_lib_bitops >= 201907L
#include <bit>

namespace bitops
{
using std::bit_ceil;
using std::bit_floor;
using std::bit_width;
using std::countl_one;
using std::countl_zero;
using std::countr_one;
using std::countr_zero;
using std::has_single_bit;
using std::popcount;
using std::rotl;
using std::rotr;
}; // namespace bitops
#else
#include <array>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace bitops
{
#if __cpp_lib_byte >= 201603L
using std::byte;
#else
enum class byte : unsigned char {};
#endif

template <typename T>
constexpr typename std::enable_if<std::is_unsigned<T>::value, int>::type
countr_zero(T x) noexcept
{
  int i = 0;
  for (; x; ++i) x <<= 1;
  return std::numeric_limits<T>::digits - i;
}
template <typename T>
constexpr typename std::enable_if<std::is_unsigned<T>::value, int>::type
countr_one(T x) noexcept
{
  return countr_zero(~x);
}
template <typename T>
constexpr typename std::enable_if<std::is_unsigned<T>::value, int>::type
countl_zero(T x) noexcept
{
  int i = 0;
  for (; x; ++i) x >>= 1;
  return std::numeric_limits<T>::digits - i;
}
template <typename T>
constexpr typename std::enable_if<std::is_unsigned<T>::value, int>::type
countl_one(T x) noexcept
{
  return countl_zero(~x);
}

template <typename T>
constexpr typename std::enable_if<std::is_unsigned<T>::value, int>::type
popcount(T x) noexcept
{
  int i = 0;
  for (; x; ++i) x &= x - 1;
  return i;
};

template <typename T>
constexpr typename std::enable_if<std::is_unsigned<T>::value, bool>::type
has_single_bit(T x) noexcept
{
  return popcount(x) == 1;
};

template <typename T>
constexpr typename std::enable_if<std::is_unsigned<T>::value, int>::type
bit_width(T x) noexcept
{
  int i = 0;
  for (; x; ++i) x >>= 1;
  return i;
}

template <typename T>
constexpr typename std::enable_if<std::is_unsigned<T>::value, T>::type
bit_ceil(T x) noexcept
{
  if (x == 1) return 1;
  return (T)1 << (bit_width(x - 1));
};

template <typename T>
constexpr typename std::enable_if<std::is_unsigned<T>::value, T>::type
bit_floor(T x) noexcept
{
  if (x == 0) return 0;
  return (T)1 << (std::numeric_limits<T>::digits - countl_zero(x - 1));
};

template <typename T>
constexpr typename std::enable_if<std::is_unsigned<T>::value, T>::type
rotl(T x, int s) noexcept
{
  unsigned r = s;
  r %= std::numeric_limits<T>::digits;
  return (x << r) | (x >> (std::numeric_limits<T>::digits - r));
}

template <typename T>
constexpr typename std::enable_if<std::is_unsigned<T>::value, T>::type
rotr(T x, int s) noexcept
{
  unsigned r = s;
  r %= std::numeric_limits<T>::digits;
  return (x >> r) | (x << (std::numeric_limits<T>::digits - r));
}

}; // namespace bitops
#endif
