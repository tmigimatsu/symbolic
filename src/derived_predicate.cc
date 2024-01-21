/**
 * derived_predicate.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: April 28, 2020
 * Authors: Toki Migimatsu
 */

#include "symbolic/derived_predicate.h"

#include <VAL/ptree.h>

#include "symbolic/pddl.h"

namespace symbolic {

DerivedPredicate::DerivedPredicate(const Pddl& pddl,
                                   const VAL::derivation_rule* symbol)
    : Action(pddl), symbol_(symbol) {
  name_ = symbol_->get_head()->head->getName();
  parameters_ = Object::CreateList(pddl, symbol_->get_head()->args);
  param_gen_ = ParameterGenerator(pddl, parameters_);
  Preconditions_ = Formula(pddl, symbol_->get_body(), parameters_);
}

bool DerivedPredicate::Apply(State* state) const {
  bool is_changed = false;
  bool is_iter_changed = true;
  while (is_iter_changed) {
    // Keep iterating until predicate converges
    is_iter_changed = false;
    for (const std::vector<Object>& arguments : parameter_generator()) {
      is_iter_changed |= IsValid(*state, arguments)
                             ? state->emplace(name(), arguments)
                             : state->erase(Proposition(name(), arguments));
    }
    is_changed |= is_iter_changed;
  }
  return is_changed;
}

bool DerivedPredicate::Apply(const std::vector<DerivedPredicate>& predicates,
                             State* state) {
  bool is_changed = false;
  bool is_iter_changed = true;
  while (is_iter_changed) {
    // Keep iterating until predicates converge
    is_iter_changed = false;
    for (const DerivedPredicate& pred : predicates) {
      is_iter_changed |= pred.Apply(state);
    }
    is_changed |= is_iter_changed;
  }
  return is_changed;
}

State DerivedPredicate::Apply(const State& state,
                              const std::vector<DerivedPredicate>& predicates) {
  State next_state = state;
  Apply(predicates, &next_state);
  return next_state;
}

}  // namespace symbolic
