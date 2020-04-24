/**
 * axiom.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: April 23, 2020
 * Authors: Toki Migimatsu
 */

#include "symbolic/axiom.h"

#include "symbolic/normal_form.h"

namespace symbolic {

Axiom::Axiom(const Pddl& pddl, const VAL::operator_* symbol)
    : Action(pddl, symbol) {
  for (const std::vector<Object>& arguments : parameter_generator()) {
    const std::optional<DisjunctiveFormula> dnf = DisjunctiveFormula::Create(
        pddl, preconditions(), parameters(), arguments);
    if (!dnf.has_value()) continue;
    arguments_.push_back(arguments);
  }
}

bool Axiom::IsConsistent(const State& state) const {
  State test_state = state;
  for (const std::vector<Object>& args : arguments_) {
    // Test if context is valid
    if (!IsValid(state, args)) continue;

    // Test if implies is valid (state shouldn't change)
    if (Apply(args, &test_state)) return false;
  }
  return true;
}

}  // namespace symbolic
