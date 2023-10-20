#pragma once
#include <cassert>
#include <cstddef>
#include <new>
#include <utility>

namespace util
{
template <class T, std::size_t N>
class ring_buffer
{
private:
  alignas(T) unsigned char _buffer[N][sizeof(T)];

public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type &;
  using const_reference = value_type const &;
  using pointer = value_type *;
  using const_pointer = value_type const *;

private:
  pointer _front, _back;
  size_type _size;

public:
  ring_buffer() : _front((T *)_buffer[0]), _back((T *)_buffer[0]), _size(0) {}
  ~ring_buffer()
  {
    while (!empty()) pop();
  }
  constexpr bool empty() const { return _size == 0; }
  constexpr size_type size() const { return _size; }
  constexpr size_type capacity() const { return N; }

  constexpr reference front() { return *_front; }
  constexpr const_reference front() const { return front(); }
  constexpr reference back()
  {
    return _back == (T *)_buffer[0] ? *(T *)_buffer[N - 1] : *(_back - 1);
  }
  constexpr const_reference back() const { return back(); }

  template <class... Args>
  constexpr void push(Args &&...args)
  {
    new (_back) value_type(std::forward<Args...>(args...));
    ++_size;
    ++_back;
    if (_back == (T *)_buffer[N]) _back = (T *)_buffer[0];
    assert(_size <= N);
  }

  constexpr void pop()
  {
    front().~T();
    --_size;
    ++_front;
    if (_front == (T *)_buffer[N]) _front = (T *)_buffer[0];
  }
};
} // namespace util
