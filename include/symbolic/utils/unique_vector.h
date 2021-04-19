/**
 * unique_vector.h
 *
 * Copyright 2021. All Rights Reserved.
 *
 * Created: April 15, 2021
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_UTILS_UNIQUE_VECTOR_H_
#define SYMBOLIC_UTILS_UNIQUE_VECTOR_H_

#include <algorithm>  // std::is_sorted, std::lower_bound, std::sort
#include <cassert>    // assert
#include <vector>     // std::vector

namespace symbolic {

/**
 * Set implemented as a sorted vector of unique elements.
 */
template <typename T>
class UniqueVector : public std::vector<T> {
 public:
  UniqueVector() = default;

  UniqueVector(std::initializer_list<T> l);

  /**
   * Returns whether the vector contains the given value.
   */
  template<typename T_query>
  bool contains(const T_query& val) const;

  /**
   * Inserts a value into the vector and returns whether or not the vector has
   * changed.
   */
  template<typename T_query>
  bool insert(const T_query& val);
  bool insert(T&& val);

  /**
   * Emplaces a value into the vector and returns whether or not the vector has
   * changed.
   */
  template <class... Args>
  bool emplace(Args&&... args) {
    return insert(T(args...));
  }

  /**
   * Removes a value from the vector and returns whether or not the vector has
   * changed.
   */
  template<typename T_query>
  bool erase(const T_query& val);

 private:
  using Base = std::vector<T>;
};

template<typename T>
UniqueVector<T>::UniqueVector(std::initializer_list<T> l) : Base(l) {
  std::sort(Base::begin(), Base::end());
  auto last = std::unique(Base::begin(), Base::end());
  Base::erase(last, Base::end());
  assert(std::is_sorted(Base::begin(), Base::end()));
}

template <typename T>
template<typename T_query>
bool UniqueVector<T>::contains(const T_query& val) const {
  assert(std::is_sorted(Base::begin(), Base::end()));
  const auto it = std::lower_bound(Base::begin(), Base::end(), val);
  return it != Base::end() && *it == val;
}

template <typename T>
template<typename T_query>
bool UniqueVector<T>::insert(const T_query& val) {
  assert(std::is_sorted(Base::begin(), Base::end()));
  const auto it = std::lower_bound(Base::begin(), Base::end(), val);
  if (it != Base::end() && *it == val) return false;

  Base::emplace(it, val);
  return true;
}

template <typename T>
bool UniqueVector<T>::insert(T&& val) {
  assert(std::is_sorted(Base::begin(), Base::end()));
  const auto it = std::lower_bound(Base::begin(), Base::end(), val);
  if (it != Base::end() && *it == val) return false;

  Base::insert(it, std::move(val));
  return true;
}

template <typename T>
template<typename T_query>
bool UniqueVector<T>::erase(const T_query& val) {
  assert(std::is_sorted(Base::begin(), Base::end()));
  const auto it = std::lower_bound(Base::begin(), Base::end(), val);
  if (it == Base::end() || *it != val) return false;
  Base::erase(it);
  return true;
}

}  // namespace symbolic

#endif  // SYMBOLIC_UTILS_UNIQUE_VECTOR_H_
