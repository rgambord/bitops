#pragma once
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <type_traits>

#include "util/fitted_int/fitted_int.hh"
#include "util/bitops/bitops.hh"
#include "util/adaptors/reverse_adaptor.hh"

template <std::size_t N>
class bitmap
{
public:
  using id_type = FittedInt::uint_fastX_t<N>;
private:
  /*
  using ChunkT = typename std::conditional<N <= 8, std::uint_fast8_t,
                 typename std::conditional<N <= 16, std::uint_fast16_t,
                 typename std::conditional<N <= 32, std::uint_fast32_t, std::uint_fast64_t>::type>::type>::type;
                 */
  using ChunkT = std::uint64_t;
  constexpr static auto CHUNK_BITS = std::numeric_limits<ChunkT>::digits;
  constexpr static auto CHUNK_COUNT = (N + CHUNK_BITS - 1) / CHUNK_BITS;
  constexpr static auto PAD_BITS = (CHUNK_BITS * CHUNK_COUNT) - N;
  constexpr static auto PAD_MASK = (ChunkT) ((~(ChunkT)0) >> PAD_BITS);

  std::array<ChunkT, CHUNK_COUNT> _bit_array;
  
  class BitId;

  class ChunkId
  {
  private:
    using T = FittedInt::uint_fastX_t<CHUNK_COUNT>;
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
    using T = FittedInt::uint_fastX_t<CHUNK_BITS>;
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
    constexpr BitId(ChunkId id, ChunkOffset offset) : _val(id * CHUNK_BITS + offset) {}
    constexpr operator T &() { return _val; }
    constexpr operator T const &() const { return _val; }
  };

public:
  constexpr bitmap() : _bit_array{} {}

  explicit constexpr bitmap(std::bitset<N> const &other) : _bit_array{}
  {
    static_assert(std::numeric_limits<unsigned long long>::digits >= CHUNK_BITS);
    unsigned long long block;
    std::bitset<N> mask;
    mask.flip() >>= (N - CHUNK_BITS);
    std::bitset<N> copy(other);
    for (ChunkT &chunk : _bit_array) {
      chunk = (copy & mask).to_ullong();
      copy >>= CHUNK_BITS;
    }
  }

  explicit constexpr operator std::bitset<N>() const
  {
    static_assert(std::numeric_limits<unsigned long long>::digits >= CHUNK_BITS);
    std::bitset<N> ret;

    for (ChunkT const& chunk : reverse_adaptor(_bit_array)) {
      ret <<= CHUNK_BITS;
      ret |= chunk;
    }
    return ret;
  }
public:

  constexpr std::size_t size() const { return N; }

  constexpr bitmap &set(BitId bit, bool val = true)
  {
    if (bit >= N) throw std::range_error("invalid index");
    (*this)[bit] = val;
    return *this;
  }

  constexpr bitmap &set() noexcept
  {
    for (auto &chunk : _bit_array) chunk = ~(ChunkT)0;
    _bit_array.back() &= PAD_MASK;
    return *this;
  }

  constexpr bitmap &reset() noexcept
  {
    for (auto &chunk : _bit_array) chunk = 0;
    return *this;
  }
  constexpr bitmap &reset(BitId bit)
  {
    set(bit, false);
    return *this;
  }

  constexpr bitmap &flip(BitId bit)
  {
    set(bit, ~(*this)[bit]);
    return *this;
  }

  constexpr bitmap &flip() noexcept
  {
    for (auto &chunk : _bit_array) chunk = ~chunk;
    _bit_array.back() &= PAD_MASK;
    return *this;
  }

  constexpr bool test(BitId bit) const
  {
    if (bit >= N) throw std::range_error("invalid index");
    return (*this)[bit];
  }

  constexpr bool operator==(bitmap const &other) const noexcept
  {
    return std::equal(_bit_array.cbegin(), _bit_array.cend(), other._bit_array.cbegin());
  }

  constexpr bool operator!=(bitmap const &other) const noexcept { return !(*this == other); }

  constexpr bitmap operator~() const noexcept { return bitmap(*this).flip(); }

  constexpr bitmap &operator&=(bitmap const &other) noexcept
  {
    std::transform(_bit_array.begin(), _bit_array.end(), other._bit_array.begin(),
                   _bit_array.begin(), std::bit_and());
    return *this;
  }
  constexpr bitmap &operator|=(bitmap const &other) noexcept
  {
    std::transform(_bit_array.begin(), _bit_array.end(), other._bit_array.begin(),
                   _bit_array.begin(), std::bit_or());
    return *this;
  }
  constexpr bitmap &operator^=(bitmap const &other) noexcept
  {
    std::transform(_bit_array.begin(), _bit_array.end(), other._bit_array.begin(),
                   _bit_array.begin(), std::bit_xor());
    return *this;
  }

  constexpr bool any() const noexcept
  {
    for (auto const v : _bit_array) if (v) return true;
    return false;
  }
  constexpr bool none() const noexcept { return !any(); }
  constexpr bool all() const noexcept
  {
    return _bit_array.back() == PAD_MASK &&
           std::all_of(_bit_array.begin(), _bit_array.end() - 1, [](auto x) { return !~x; });
  }

  constexpr BitId count() const noexcept { 
    BitId cnt = 0;
    for (auto const&v : _bit_array) cnt += popcount(v);
    return cnt;
  }

  class bit_proxy
  {
    friend bitmap;

  private:
    ChunkT &_chunk;
    ChunkT _mask;
    constexpr bit_proxy(ChunkT &chunk, ChunkT mask) : _chunk(chunk), _mask(mask) {}
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
  constexpr bit_proxy operator[](BitId id)
  {
    return bit_proxy(_bit_array[ChunkId(id)], ChunkOffset(id));
  }
  constexpr bool operator[](BitId id) const
  {
    ChunkT mask = ChunkT(1) << ChunkOffset(id);
    return (bool) (ChunkT) (_bit_array[ChunkId(id)] & mask);
  }

  class biterator
  {
    friend bitmap;

  public:
    using difference_type = BitId;
    using value_type = BitId;
    using pointer = value_type *;
    using reference = value_type;
    using iterator_category = std::bidirectional_iterator_tag;

  private:
    bitmap &_ref;
    ChunkId _id;
    ChunkOffset _offset;
    constexpr biterator(bitmap &ref, ChunkId id, ChunkOffset offset)
        : _ref(ref), _id(id), _offset(offset)
    {
    }
    constexpr biterator(bitmap &ref, BitId id) : _ref(ref), _id(id), _offset(id) {}

  public:
    constexpr reference operator*() { return reference(_id, _offset); }

    biterator &operator++()
    {
      if (_id >= CHUNK_COUNT) throw std::range_error("iterate past end");
      ChunkT chunk = _ref._bit_array[_id] >> (_offset + 1);
      if ((_offset + 1 < CHUNK_BITS) && chunk) {
        _offset += countr_zero(chunk) + 1;
      } else {
        _offset = 0; /* IMPORTANT */
        while (++_id < CHUNK_COUNT) {
          chunk = _ref._bit_array[_id];
          if (chunk) {
            _offset = countr_zero(chunk);
            break;
          }
        }
      }
      return *this;
    }
    biterator operator++(int) { return ++biterator(*this); }

    biterator &operator--()
    {
      ChunkT chunk = _ref._bit_array[_id] << (CHUNK_BITS - _offset);
      if (_offset && chunk) {
        _offset -= countl_zero(chunk);
      } else {
        _offset = 0; /* IMPORTANT */
        while (_id-- > 0) {
          chunk = _ref._bit_array[_id];
          if (chunk) {
            _offset = countl_zero(chunk);
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
      return std::addressof(_ref) == std::addressof(other._ref) && _id == other._id &&
             _offset == other._offset;
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
    for (; id < CHUNK_COUNT; ++id) {
      ChunkT chunk = _bit_array[id];
      if (chunk) {
        offset = countr_zero(chunk);
        break;
      }
    }
    return biterator(*this, id, offset);
  }

  biterator end() { return biterator(*this, ChunkId(CHUNK_COUNT), 0); }

  template <class CharT = char, class Traits = std::char_traits<CharT>,
            class Allocator = std::allocator<CharT>>
  std::basic_string<CharT, Traits, Allocator> to_string(CharT zero = CharT('0'),
                                                        CharT one = CharT('1')) const
  {
    return std::bitset<N>(*this).template to_string<CharT, Traits, Allocator>(zero, one);
  }
};

template <std::size_t N>
bitmap<N>
operator&(bitmap<N> const &lhs, bitmap<N> const &rhs)
{
  return bitmap(lhs) &= rhs;
}
template <std::size_t N>
bitmap<N>
operator|(bitmap<N> const &lhs, bitmap<N> const &rhs)
{
  return bitmap(lhs) |= rhs;
}
template <std::size_t N>
bitmap<N>
operator^(bitmap<N> const &lhs, bitmap<N> const &rhs)
{
  return bitmap(lhs) ^= rhs;
}

template <class CharT, class Traits, std::size_t N>
std::basic_ostream<CharT, Traits> &
operator<<(std::basic_ostream<CharT, Traits> &os, const bitmap<N> &x)
{
  os << std::bitset<N>(x);
  return os;
}

template <class CharT, class Traits, std::size_t N>
std::basic_istream<CharT, Traits> &
operator>>(std::basic_istream<CharT, Traits> &is, bitmap<N> &x)
{
  std::bitset<N> s;
  is >> s;
  x = s;
  return is;
}
