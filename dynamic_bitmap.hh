/* Almost identical to bitmap, except practically none of the
 * operations can be constexpr and some of the static constexpr values
 * have to instead by constexpr instance methods
 *
 * Much slower than bitmap for small sizes (< ~512 bits)
 *
 * TODO: Merge with bitmap, with: `using dynamic_bitmap = bitmap<0>;`
 *
 * Author: Ryan Gambord <Ryan.Gambord@oregonstate.edu>
 * Date: July 26 2023
 */
#pragma once
#include <algorithm>
#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "util/adaptors/reverse.hh"
#include "util/bit.hh"
#include "util/fitted_int.hh"

namespace util
{
class dynamic_bitmap
{
public:
  using id_type = std::size_t;

private:
  std::size_t _size;
  using ChunkT = std::uint64_t;
  constexpr static auto CHUNK_BITS = std::numeric_limits<ChunkT>::digits;
  constexpr auto CHUNK_COUNT() const
  {
    return (_size + CHUNK_BITS - 1) / CHUNK_BITS;
  }
  constexpr auto PAD_BITS() const
  {
    return (CHUNK_BITS * CHUNK_COUNT()) - _size;
  }
  constexpr auto PAD_MASK() const
  {
    return (ChunkT)((~(ChunkT)0) >> PAD_BITS());
  }

  std::vector<ChunkT> _bit_vec;

  class BitId;

  class ChunkId
  {
  private:
    using T = id_type;
    T _val;

  public:
    explicit constexpr ChunkId(T val) : _val(val) {}
    constexpr ChunkId(BitId id) : _val(id / CHUNK_BITS){};
    constexpr operator T &() { return _val; }
    constexpr operator T const &() const { return _val; }
  };

  class ChunkOffset
  {
  private:
    using T = fitted_int::uint_fastX_t<CHUNK_BITS>;
    T _val;

  public:
    constexpr ChunkOffset(T val) : _val(val) {}
    constexpr ChunkOffset(BitId id) : _val(id % CHUNK_BITS) {}
    constexpr operator T &() { return _val; }
    constexpr operator T const &() const { return _val; }
  };

  class BitId
  {
  private:
    using T = id_type;
    T _val;

  public:
    constexpr BitId(T val = 0) : _val(val) {}
    constexpr BitId(ChunkId id, ChunkOffset offset)
        : _val(id * CHUNK_BITS + offset)
    {
    }
    constexpr operator T &() { return _val; }
    constexpr operator T const &() const { return _val; }
  };

public:
  dynamic_bitmap(std::size_t size) : _bit_vec{}, _size(size)
  {
    _bit_vec.resize(CHUNK_COUNT());
  }

  template <std::size_t N>
  explicit constexpr dynamic_bitmap(std::bitset<N> const &other)
      : _bit_vec{}, _size(N)
  {
    _bit_vec.resize(CHUNK_COUNT());
    static_assert(std::numeric_limits<unsigned long long>::digits >=
                  CHUNK_BITS);
    std::bitset<N> mask;
    mask.flip() >>= (N - CHUNK_BITS);
    std::bitset<N> copy(other);
    for (ChunkT &chunk : _bit_vec) {
      chunk = (copy & mask).to_ullong();
      copy >>= CHUNK_BITS;
    }
  }

  void resize(std::size_t size)
  {
    _size = size;
    _bit_vec.resize(CHUNK_COUNT());
  }

public:
  constexpr std::size_t size() const { return _size; }

  dynamic_bitmap &set(BitId bit, bool val = true)
  {
    if (bit >= _size) throw std::range_error("invalid index");
    (*this)[bit] = val;
    return *this;
  }

  dynamic_bitmap &set() noexcept
  {
    for (auto &chunk : _bit_vec) chunk = ~(ChunkT)0;
    _bit_vec.back() &= PAD_MASK();
    return *this;
  }

  dynamic_bitmap &reset() noexcept
  {
    for (auto &chunk : _bit_vec) chunk = 0;
    return *this;
  }

  dynamic_bitmap &reset(BitId bit)
  {
    set(bit, false);
    return *this;
  }

  dynamic_bitmap &flip(BitId bit)
  {
    set(bit, ~(*this)[bit]);
    return *this;
  }

  dynamic_bitmap &flip() noexcept
  {
    for (auto &chunk : _bit_vec) chunk = ~chunk;
    _bit_vec.back() &= PAD_MASK();
    return *this;
  }

  bool test(BitId bit) const
  {
    if (bit >= _size) throw std::range_error("invalid index");
    return (*this)[bit];
  }

  bool operator==(dynamic_bitmap const &other) const noexcept
  {
    return std::equal(_bit_vec.cbegin(), _bit_vec.cend(),
                      other._bit_vec.cbegin());
  }

  bool operator!=(dynamic_bitmap const &other) const noexcept
  {
    return !(*this == other);
  }

  dynamic_bitmap operator~() const noexcept
  {
    return dynamic_bitmap(*this).flip();
  }

  dynamic_bitmap &operator&=(dynamic_bitmap const &other) noexcept
  {
    std::transform(_bit_vec.begin(), _bit_vec.end(), other._bit_vec.begin(),
                   _bit_vec.begin(), std::bit_and());
    return *this;
  }
  dynamic_bitmap &operator|=(dynamic_bitmap const &other) noexcept
  {
    std::transform(_bit_vec.begin(), _bit_vec.end(), other._bit_vec.begin(),
                   _bit_vec.begin(), std::bit_or());
    return *this;
  }
  dynamic_bitmap &operator^=(dynamic_bitmap const &other) noexcept
  {
    std::transform(_bit_vec.begin(), _bit_vec.end(), other._bit_vec.begin(),
                   _bit_vec.begin(), std::bit_xor());
    return *this;
  }

  bool any() const noexcept
  {
    return std::any_of(_bit_vec.begin(), _bit_vec.end(),
                       [](auto x) { return x; });
  }
  bool none() const noexcept { return !any(); }
  bool all() const noexcept
  {
    return _bit_vec.back() == PAD_MASK() &&
           std::all_of(_bit_vec.begin(), _bit_vec.end() - 1,
                       [](auto x) { return !~x; });
  }

  BitId count() const noexcept
  {
    BitId cnt = 0;
    for (auto const &v : _bit_vec) cnt += bitops::popcount(v);
    return cnt;
  }

  class bit_proxy
  {
    friend dynamic_bitmap;

  private:
    ChunkT &_chunk;
    ChunkT _mask;
    constexpr bit_proxy(ChunkT &chunk, ChunkT mask) : _chunk(chunk), _mask(mask)
    {
    }
    constexpr bit_proxy(ChunkT &chunk, ChunkOffset offset)
        : _chunk(chunk), _mask(ChunkT(1) << offset)
    {
    }

  public:
    constexpr operator bool() const noexcept { return _chunk & _mask; }
    constexpr bit_proxy &operator=(bool val) noexcept
    {
      if (val) _chunk |= _mask;
      else _chunk &= ~_mask;
      return *this;
    }
    constexpr bool operator~() const noexcept { return !*this; }
    constexpr bit_proxy &flip() noexcept { return *this = ~*this; }
  };

public:
  bit_proxy operator[](BitId id)
  {
    return bit_proxy(_bit_vec[ChunkId(id)], ChunkOffset(id));
  }
  bool operator[](BitId id) const
  {
    ChunkT mask = ChunkT(1) << ChunkOffset(id);
    return (bool)(ChunkT)(_bit_vec[ChunkId(id)] & mask);
  }

  class biterator
  {
    friend dynamic_bitmap;

  public:
    using difference_type = BitId;
    using value_type = BitId;
    using pointer = value_type *;
    using reference = value_type;
    using iterator_category = std::bidirectional_iterator_tag;

  private:
    dynamic_bitmap &_ref;
    ChunkId _id;
    ChunkOffset _offset;
    constexpr biterator(dynamic_bitmap &ref, ChunkId id, ChunkOffset offset)
        : _ref(ref), _id(id), _offset(offset)
    {
    }
    constexpr biterator(dynamic_bitmap &ref, BitId id)
        : _ref(ref), _id(id), _offset(id)
    {
    }

  public:
    constexpr reference operator*() { return reference(_id, _offset); }

    biterator &operator++()
    {
      if (_id >= _ref.CHUNK_COUNT()) throw std::range_error("iterate past end");
      ChunkT chunk = _ref._bit_vec[_id] >> (_offset + 1);
      if ((_offset + 1 < CHUNK_BITS) && chunk) {
        _offset += bitops::countr_zero(chunk) + 1;
      } else {
        _offset = 0; /* IMPORTANT */
        while (++_id < _ref.CHUNK_COUNT()) {
          chunk = _ref._bit_vec[_id];
          if (chunk) {
            _offset = bitops::countr_zero(chunk);
            break;
          }
        }
      }
      return *this;
    }
    biterator operator++(int) { return ++biterator(*this); }

    biterator &operator--()
    {
      ChunkT chunk = _ref._bit_vec[_id] << (CHUNK_BITS - _offset);
      if (_offset && chunk) {
        _offset -= bitops::countl_zero(chunk);
      } else {
        _offset = 0; /* IMPORTANT */
        while (_id-- > 0) {
          chunk = _ref._bit_vec[_id];
          if (chunk) {
            _offset = bitops::countl_zero(chunk);
            break;
          }
          throw std::range_error("iterate before begin");
        }
      }
      return *this;
    }
    biterator operator--(int) { return --biterator(*this); }

    constexpr bool operator==(biterator const &other) const noexcept
    {
      return std::addressof(_ref) == std::addressof(other._ref) &&
             _id == other._id && _offset == other._offset;
    }
    constexpr bool operator!=(biterator const &other) const noexcept
    {
      return !operator==(other);
    }
  };

  biterator begin()
  {
    ChunkId id(0);
    ChunkOffset offset(0);
    for (; id < CHUNK_COUNT(); ++id) {
      ChunkT chunk = _bit_vec[id];
      if (chunk) {
        offset = bitops::countr_zero(chunk);
        break;
      }
    }
    return biterator(*this, id, offset);
  }

  biterator end() { return biterator(*this, ChunkId(CHUNK_COUNT()), 0); }

  template <class CharT = char, class Traits = std::char_traits<CharT>,
            class Allocator = std::allocator<CharT>>
  std::basic_string<CharT, Traits, Allocator>
  to_string(CharT zero = CharT('0'), CharT one = CharT('1')) const
  {
    return std::accumulate(
               _bit_vec.rbegin(), _bit_vec.rend(),
               std::basic_string<CharT, Traits, Allocator>(),
               [&](std::basic_string<CharT, Traits, Allocator> const &acc,
                   ChunkT const &i) {
                 return acc + std::bitset<CHUNK_BITS>(i)
                                  .template to_string<CharT, Traits, Allocator>(
                                      zero, one);
               })
        .substr(CHUNK_BITS * CHUNK_COUNT() - _size);
  }
};

dynamic_bitmap
operator&(dynamic_bitmap const &lhs, dynamic_bitmap const &rhs)
{
  return dynamic_bitmap(lhs) &= rhs;
}
dynamic_bitmap
operator|(dynamic_bitmap const &lhs, dynamic_bitmap const &rhs)
{
  return dynamic_bitmap(lhs) |= rhs;
}
dynamic_bitmap
operator^(dynamic_bitmap const &lhs, dynamic_bitmap const &rhs)
{
  return dynamic_bitmap(lhs) ^= rhs;
}

template <class CharT, class Traits>
std::basic_ostream<CharT, Traits> &
operator<<(std::basic_ostream<CharT, Traits> &os, const dynamic_bitmap &x)
{
  os << x.to_string();
  return os;
}
} // namespace util
