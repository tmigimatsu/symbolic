/**
 * state.h
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: April 23, 2020
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_STATE_H_
#define SYMBOLIC_STATE_H_

// #define SYMBOLIC_STATE_USE_SET

#include <ostream>  // std::ostream

#include "symbolic/proposition.h"

#ifndef SYMBOLIC_STATE_USE_SET
#include <vector>  // std::vector
#else   // SYMBOLIC_STATE_USE_SET
#include <set>     // std::set
#endif  // SYMBOLIC_STATE_USE_SET

namespace symbolic {

#ifndef SYMBOLIC_STATE_USE_SET
class State : private std::vector<Proposition> {
  using Base = std::vector<Proposition>;
#else   // SYMBOLIC_STATE_USE_SET
class State : private std::set<Proposition> {
  using Base = std::set<Proposition>;
#endif  // SYMBOLIC_STATE_USE_SET

 public:
  using iterator = Base::iterator;
  using const_iterator = Base::const_iterator;

  State() {}
  State(std::initializer_list<Proposition> l);

  bool contains(const Proposition& prop) const;

  bool insert(const Proposition& prop);
  bool insert(Proposition&& prop);

  template <class InputIt>
  bool insert(InputIt first, InputIt last);

  template <class... Args>
  bool emplace(Args&&... args) {
    return insert(Proposition(args...));
  }

  bool erase(const Proposition& prop);

  iterator begin() { return Base::begin(); }
  iterator end() { return Base::end(); }

  const_iterator begin() const { return Base::begin(); }
  const_iterator end() const { return Base::end(); };

  size_t empty() const { return Base::empty(); }
  size_t size() const { return Base::size(); }

#ifndef SYMBOLIC_STATE_USE_SET
  void reserve(size_t size) { static_cast<Base&>(*this).reserve(size); }
#else   // SYMBOLIC_STATE_USE_SET
  void reserve(size_t size) {}
#endif  // SYMBOLIC_STATE_USE_SET

  friend bool operator==(const State& lhs, const State& rhs) {
    return static_cast<const Base&>(lhs) == static_cast<const Base&>(rhs);
  }
  friend bool operator!=(const State& lhs, const State& rhs) {
    return !(lhs == rhs);
  }

  friend bool operator<(const State& lhs, const State& rhs) {
    return static_cast<const Base&>(lhs) < static_cast<const Base&>(rhs);
  }
  friend bool operator>(const State& lhs, const State& rhs) {
    return !(lhs < rhs);
  }

  friend std::ostream& operator<<(std::ostream& os, const State& state);
};

template <class InputIt>
bool State::insert(InputIt first, InputIt last) {
  // TODO: More efficient way of inserting
  bool is_changed = false;
  for (InputIt it = first; it != last; ++it) {
    is_changed != insert(*it);
  }
  return is_changed;
}

}  // namespace symbolic

#endif  // SYMBOLIC_STATE_H_
