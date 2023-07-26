#pragma once
#include <cstdlib>
#include <initializer_list>
#include <iterator>

template <class iter>
class reverse_adaptor
{
private:
  iter _begin;
  iter _end;

public:
  template <class T>
  constexpr reverse_adaptor(T &ref) : _begin(std::end(ref)), _end(std::begin(ref))
  {
  }

  template <class T, std::size_t N>
  constexpr reverse_adaptor(T (&ref)[N]) : _begin(std::end(ref)),
      _end(std::begin(ref))
  {
  }

  template <class T>
  constexpr reverse_adaptor(std::initializer_list<T> list) : _begin(std::end(list)), _end(std::begin(list))
  {
  }
  constexpr auto begin() { return _begin; }
  constexpr auto end() { return _end; }
};

template <class T>
reverse_adaptor(std::initializer_list<T>) -> reverse_adaptor<std::reverse_iterator<decltype(std::begin(*(std::initializer_list<T>*)nullptr))>>;

template <class T, std::size_t N>
reverse_adaptor(T (&ref)[N]) -> reverse_adaptor<std::reverse_iterator<decltype(std::begin(*(T (*)[N])nullptr))>>;

template <class T>
reverse_adaptor(T&) -> reverse_adaptor<std::reverse_iterator<decltype(std::begin(*(T*)nullptr))>>;
