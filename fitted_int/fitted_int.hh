/* Fitted fixed-width integer types
 *
 * Based on implementation by linguamachina(uid 2748290) @
 * https://stackoverflow.com/a/31272357
 */
#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>
#include <type_traits>

/* clang-format off */

namespace FittedInt {

template <class T, class U = typename std::conditional<
                                 std::is_signed<T>::value, 
                                 std::intmax_t, 
                                 std::uintmax_t 
                             >::type>
constexpr bool is_in_range (U x) {
  return (x >= std::numeric_limits<T>::min())
      && (x <= std::numeric_limits<T>::max());
}

template <std::intmax_t x>
using intX_t =
    typename std::conditional<is_in_range<std::int8_t>(x),  std::int8_t,
    typename std::conditional<is_in_range<std::int16_t>(x), std::int16_t,
    typename std::conditional<is_in_range<std::int32_t>(x), std::int32_t,
    typename std::conditional<is_in_range<std::int64_t>(x), std::int64_t,
      intmax_t>::type >::type >::type >::type;

template <std::intmax_t x>
using int_fastX_t =
    typename std::conditional<is_in_range<std::int_fast8_t>(x),  std::int_fast8_t,
    typename std::conditional<is_in_range<std::int_fast16_t>(x), std::int_fast16_t,
    typename std::conditional<is_in_range<std::int_fast32_t>(x), std::int_fast32_t,
    typename std::conditional<is_in_range<std::int_fast64_t>(x), std::int_fast64_t,
      intmax_t>::type >::type >::type >::type;

template <std::intmax_t x>
using int_leastX_t =
    typename std::conditional<is_in_range<std::int_least8_t>(x),  std::int_least8_t,
    typename std::conditional<is_in_range<std::int_least16_t>(x), std::int_least16_t,
    typename std::conditional<is_in_range<std::int_least32_t>(x), std::int_least32_t,
    typename std::conditional<is_in_range<std::int_least64_t>(x), std::int_least64_t,
      intmax_t>::type >::type >::type >::type;

template <std::uintmax_t x>
using uintX_t =
    typename std::conditional<is_in_range<std::uint8_t>(x),  std::uint8_t,
    typename std::conditional<is_in_range<std::uint16_t>(x), std::uint16_t,
    typename std::conditional<is_in_range<std::uint32_t>(x), std::uint32_t,
    typename std::conditional<is_in_range<std::uint64_t>(x), std::uint64_t,
      uintmax_t>::type >::type >::type >::type;

template <std::uintmax_t x>
using uint_fastX_t =
    typename std::conditional<is_in_range<std::uint_fast8_t>(x),  std::uint_fast8_t,
    typename std::conditional<is_in_range<std::uint_fast16_t>(x), std::uint_fast16_t,
    typename std::conditional<is_in_range<std::uint_fast32_t>(x), std::uint_fast32_t,
    typename std::conditional<is_in_range<std::uint_fast64_t>(x), std::uint_fast64_t,
      uintmax_t>::type >::type >::type >::type;

template <std::uintmax_t x>
using uint_leastX_t =
    typename std::conditional<is_in_range<std::uint_least8_t>(x),  std::uint_least8_t,
    typename std::conditional<is_in_range<std::uint_least16_t>(x), std::uint_least16_t,
    typename std::conditional<is_in_range<std::uint_least32_t>(x), std::uint_least32_t,
    typename std::conditional<is_in_range<std::uint_least64_t>(x), std::uint_least64_t,
      uintmax_t>::type >::type >::type >::type;

/* clang-format on */
}; // namespace FittedInt
