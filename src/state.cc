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

#include "symbolic/pddl.h"

namespace {

using symbolic::Predicate;

std::vector<size_t> PredicateCumSum(const std::vector<Predicate>& predicates) {
  std::vector<size_t> idx_pred;
  idx_pred.reserve(predicates.size() + 1);
  idx_pred.push_back(0);
  for (const Predicate& pred : predicates) {
    const size_t idx = idx_pred.back() + pred.parameter_generator().size();
    idx_pred.push_back(idx);
  }
  return idx_pred;
}

std::unordered_map<std::string, size_t> PredicateIndices(
    const std::vector<Predicate>& predicates) {
  std::unordered_map<std::string, size_t> idx_predicates;
  for (size_t i = 0; i < predicates.size(); i++) {
    const Predicate& pred = predicates[i];
    idx_predicates[pred.name()] = i;
  }
  return idx_predicates;
}

}  // namespace

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

StateIndex::StateIndex(const std::vector<Predicate>& predicates, bool use_cache)
    : predicates_(predicates),
      idx_predicate_group_(PredicateCumSum(predicates_)),
      idx_predicates_(PredicateIndices(predicates_)),
      use_cache_(use_cache) {}

Proposition StateIndex::GetProposition(size_t idx_proposition) const {
  // Check cache
  if (use_cache_ &&
      cache_propositions_.find(idx_proposition) != cache_propositions_.end()) {
    return Proposition(cache_propositions_.at(idx_proposition));
  }

  // Find index of predicate through bisection
  // std::lower_bound returns first element >= value. Get first element > value
  // and then subtract 1.
  const auto it =
      std::lower_bound(idx_predicate_group_.begin(), idx_predicate_group_.end(),
                       idx_proposition + 1);
  const size_t idx_pred = it - idx_predicate_group_.begin() - 1;
  for (size_t i = 0; i < predicates_.size(); i++) {
  }

  // Find index of argument combination from remainder
  const size_t idx_args = idx_proposition - idx_predicate_group_[idx_pred];

  // Get proposition
  const Predicate& pred = predicates_[idx_pred];
  std::vector<Object> args = pred.parameter_generator()[idx_args];

  // Cache results
  if (use_cache_) {
    cache_propositions_[idx_proposition] = Proposition(pred.name(), args).to_string();
  }

  return Proposition(pred.name(), std::move(args));
}

size_t StateIndex::GetPropositionIndex(const Proposition& prop) const {
  // Check cache
  if (use_cache_) {
    const auto it = cache_idx_propositions_.find(prop.to_string());
    if (it != cache_idx_propositions_.end()) {
      return it->second;
    }
  }

  const size_t idx_pred = idx_predicates_.at(prop.name());

  // Get predicate
  const Predicate& pred = predicates_[idx_pred];
  const ParameterGenerator& param_gen = pred.parameter_generator();

  // Get arguments
  const size_t idx_args = param_gen.find(prop.arguments());

  const size_t idx_proposition = idx_predicate_group_[idx_pred] + idx_args;

  // Cache results
  if (use_cache_) {
    cache_idx_propositions_[prop.to_string()] = idx_proposition;
  }
  return idx_proposition;
}

// NOLINTNEXTLINE(performance-unnecessary-value-param)
State StateIndex::GetState(Eigen::Ref<const IndexedState> indexed_state) const {
  assert(indexed_state.size() == size());
  State state;

  // Iterate over nonzero elements of indexed state
  for (size_t i = 0; i < indexed_state.size(); i++) {
    if (!indexed_state(i)) continue;
    state.insert(GetProposition(i));
  }

  return state;
}

StateIndex::IndexedState StateIndex::GetIndexedState(const State& state) const {
  IndexedState indexed_state = IndexedState::Zero(size());

  // Iterate over propositions in state
  for (const Proposition& prop : state) {
    const size_t idx_prop = GetPropositionIndex(prop);
    indexed_state[idx_prop] = true;
  }

  return indexed_state;
}

}  // namespace symbolic
