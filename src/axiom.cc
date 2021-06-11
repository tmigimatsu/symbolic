/**
 * axiom.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: April 23, 2020
 * Authors: Toki Migimatsu
 */

#include "symbolic/axiom.h"

#include <sstream>  // std::stringstream

#include "symbolic/normal_form.h"

namespace symbolic_v1 {

Axiom::Axiom(const Pddl& pddl, const VAL_v1::operator_* symbol)
    : Action(pddl, symbol) {
  for (const std::vector<Object>& arguments : parameter_generator()) {
    const std::optional<DisjunctiveFormula> dnf = DisjunctiveFormula::Create(
        pddl, preconditions(), parameters(), arguments);
    if (!dnf.has_value()) continue;
    arguments_.push_back(arguments);
  }
  std::stringstream ss;
  try {
    ss << *DisjunctiveFormula::Create(pddl, preconditions(), parameters(),
                                      parameters());
  } catch (...) {
    ss << "false";
  }
  ss << " => ";
  try {
    ss << *DisjunctiveFormula::Create(pddl, postconditions(), parameters(),
                                      parameters());
  } catch (...) {
    ss << "false";
  }
  formula_ = ss.str();
}

bool Axiom::IsConsistent(const State& state) const {
  State test_state = state;
  for (const std::vector<Object>& args : arguments_) {
    // Test if context is valid
    if (!IsValid(state, args)) continue;

    // Test if implies is valid (state shouldn't change)
    try {
      if (Action::Apply(args, &test_state)) return false;
    } catch (const std::exception& e) {
      // std::cerr << e.what() << std::endl;
      return false;
    }
  }
  return true;
}

bool Axiom::IsConsistent(PartialState* state, bool* is_changed) const {
  for (const std::vector<Object>& args : arguments_) {
    // Test if context is valid
    const std::optional<bool> is_valid = IsValid(*state, args);
    if (!is_valid || !*is_valid) continue;

    // Test if implies is valid (state shouldn't change)
    try {
      const int changed = Action::Apply(args, state);
      if (changed > 1) return false;
      *is_changed |= changed > 0;
    } catch (const std::exception& e) {
      // std::cerr << e.what() << std::endl;
      return false;
    }
  }
  return true;
}

bool Axiom::IsConsistent(const PartialState& state) const {
  PartialState test_state = state;

  // Keep testing axioms until state doesn't change for a full iteration
  bool is_changed = true;
  while (is_changed) {
    is_changed = false;
    if (!IsConsistent(&test_state, &is_changed)) return false;
  }
  return true;
}

bool Axiom::IsConsistent(const std::vector<Axiom>& axioms,
                         const PartialState& state) {
  PartialState test_state = state;

  // Keep testing axioms until state doesn't change for a full iteration
  bool is_changed = true;
  while (is_changed) {
    is_changed = false;
    for (const Axiom& axiom : axioms) {
      if (!axiom.IsConsistent(&test_state, &is_changed)) return false;
    }
  }
  return true;
}

State Axiom::Apply(const State& state) const {
  State state_next = state;
  Apply(&state_next);
  return state_next;
}

bool Axiom::Apply(State* state) const {
  bool changed = false;
  for (const std::vector<Object>& args : arguments_) {
    // Test if context is valid
    if (!IsValid(*state, args)) continue;

    // Apply implies
    changed |= Action::Apply(args, state);
  }
  return changed;
}

PartialState Axiom::Apply(const PartialState& state) const {
  PartialState state_next = state;
  Apply(&state_next);
  return state_next;
}

int Axiom::Apply(PartialState* state) const {
  int changed = 0;
  for (const std::vector<Object>& args : arguments_) {
    // Test if context is valid
    const std::optional<bool> is_valid = IsValid(*state, args);
    if (!is_valid || !*is_valid) continue;

    // Apply implies
    changed = std::max(changed, Action::Apply(args, state));
  }
  return changed;
}

std::ostream& operator<<(std::ostream& os, const Axiom& axiom) {
  os << "axiom(";
  std::string separator;
  for (const Object& param : axiom.parameters()) {
    os << separator << param;
    if (separator.empty()) separator = ", ";
  }
  os << "): " << axiom.formula_;
  return os;
}

}  // namespace symbolic_v1
