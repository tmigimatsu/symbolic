/**
 * derived_predicate.h
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: April 28, 2020
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_DERIVED_PREDICATE_H_
#define SYMBOLIC_DERIVED_PREDICATE_H_

#include "symbolic/action.h"

namespace VAL_v1 {

class derivation_rule;

}  // namespace VAL_v1

namespace symbolic_v1 {

class DerivedPredicate : public Action {
 public:
  DerivedPredicate(const Pddl& pddl, const VAL_v1::derivation_rule* symbol);

  const VAL_v1::derivation_rule* symbol() const { return symbol_; }

  const VAL_v1::effect_lists* postconditions() const = delete;

  bool Apply(State* state) const;

  State Apply(const State& state, const std::vector<Object>& arguments) const = delete;

  bool Apply(const std::vector<Object>& arguments, State* state) const = delete;

  static State Apply(const State& state, const std::vector<DerivedPredicate>& predicates);

  static bool Apply(const std::vector<DerivedPredicate>& predicates, State* state);

 private:
  const VAL_v1::derivation_rule* symbol_;
};

}  // namespace symbolic_v1

#endif  // SYMBOLIC_DERIVED_PREDICATE_H_
