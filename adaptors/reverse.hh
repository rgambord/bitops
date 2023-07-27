/* Simple reverse iteration adaptor
 *
 * for (... : adaptor::reverse{...})
 *
 * Author: Ryan Gambord <Ryan.Gambord@oregonstate.edu>
 * Date: Jul 26 2023
 */
#pragma once
#include <cstdlib>
#include <initializer_list>
#include <iterator>

namespace adaptor {

template <class iter>
class reverse
{
private:
  iter _begin;
  iter _end;

public:
  template <class T>
  constexpr reverse(T &ref) : _begin(std::end(ref)), _end(std::begin(ref))
  {
  }

  template <class T, std::size_t N>
  constexpr reverse(T (&ref)[N]) : _begin(std::end(ref)),
      _end(std::begin(ref))
  {
  }

  template <class T>
  constexpr reverse(std::initializer_list<T> list) : _begin(std::end(list)), _end(std::begin(list))
  {
  }
  constexpr auto begin() { return _begin; }
  constexpr auto end() { return _end; }
};

template <class T>
reverse(std::initializer_list<T>) -> reverse<std::reverse_iterator<decltype(std::begin(*(std::initializer_list<T>*)nullptr))>>;

template <class T, std::size_t N>
reverse(T (&ref)[N]) -> reverse<std::reverse_iterator<decltype(std::begin(*(T (*)[N])nullptr))>>;

template <class T>
reverse(T&) -> reverse<std::reverse_iterator<decltype(std::begin(*(T*)nullptr))>>;
};
