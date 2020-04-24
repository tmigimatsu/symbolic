/**
 * state.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: April 23, 2020
 * Authors: Toki Migimatsu
 */

#include "symbolic/state.h"

#include <algorithm>  // std::lower_bound, std::sort
#include <cassert>    // assert

namespace symbolic {

State::State(std::initializer_list<Proposition> l) : Base(l) {
#ifndef SYMBOLIC_STATE_USE_SET
  std::sort(begin(), end());
  auto last = std::unique(begin(), end());
  Base::erase(last, end());
  assert(std::is_sorted(begin(), end()));
#endif  // SYMBOLIC_STATE_USE_SET
}

bool State::contains(const Proposition& prop) const {
#ifndef SYMBOLIC_STATE_USE_SET
  assert(std::is_sorted(begin(), end()));
  const auto it = std::lower_bound(begin(), end(), prop);
  return it != end() && *it == prop;
#else   // SYMBOLIC_STATE_USE_SET
  return find(prop) != end();
#endif  // SYMBOLIC_STATE_USE_SET
}

bool State::insert(const Proposition& prop) {
#ifndef SYMBOLIC_STATE_USE_SET
  assert(std::is_sorted(begin(), end()));
  const auto it = std::lower_bound(begin(), end(), prop);
  if (it != end() && *it == prop) return false;
  Base::insert(it, prop);
  return true;
#else   // SYMBOLIC_STATE_USE_SET
  return Base::insert(prop).second;
#endif  // SYMBOLIC_STATE_USE_SET
}

bool State::insert(Proposition&& prop) {
#ifndef SYMBOLIC_STATE_USE_SET
  assert(std::is_sorted(begin(), end()));
  const auto it = std::lower_bound(begin(), end(), prop);
  if (it != end() && *it == prop) return false;
  Base::insert(it, std::move(prop));
  return true;
#else   // SYMBOLIC_STATE_USE_SET
  return Base::insert(std::move(prop)).second;
#endif  // SYMBOLIC_STATE_USE_SET
}

bool State::erase(const Proposition& prop) {
#ifndef SYMBOLIC_STATE_USE_SET
  assert(std::is_sorted(begin(), end()));
  const auto it = std::lower_bound(begin(), end(), prop);
  if (it == end() || *it != prop) return false;
  Base::erase(it);
  return true;
#else   // SYMBOLIC_STATE_USE_SET
  return Base::erase(prop) > 0;
#endif  // SYMBOLIC_STATE_USE_SET
}

std::ostream& operator<<(std::ostream& os, const State& state) {
  std::string delimiter;
  os << "{ ";
  for (const Proposition& prop : state) {
    os << delimiter << prop;
    if (delimiter.empty()) delimiter = ", ";
  }
  os << " }";
  return os;
}

}  // namespace symbolic
