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

namespace VAL {

class derivation_rule;

}  // namespace VAL

namespace symbolic {

class DerivedPredicate : public Action {
 public:
  DerivedPredicate(const Pddl& pddl, const VAL::derivation_rule* symbol);

  const VAL::derivation_rule* symbol() const { return symbol_; }

  const VAL::effect_lists* postconditions() const = delete;

  bool Apply(State* state) const;

  State Apply(const State& state, const std::vector<Object>& arguments) const = delete;

  bool Apply(const std::vector<Object>& arguments, State* state) const = delete;

  static State Apply(const State& state, const std::vector<DerivedPredicate>& predicates);

  static bool Apply(const std::vector<DerivedPredicate>& predicates, State* state);

 private:
  const VAL::derivation_rule* symbol_;
};

}  // namespace symbolic

#endif  // SYMBOLIC_DERIVED_PREDICATE_H_
