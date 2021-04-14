/**
 * axiom.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: April 23, 2020
 * Authors: Toki Migimatsu
 */

#include "symbolic/axiom.h"

#include <VAL/ptree.h>

#include <sstream>  // std::stringstream

#include "symbolic/normal_form.h"

namespace {

using ::symbolic::DisjunctiveFormula;
using ::symbolic::Formula;
using ::symbolic::Object;
using ::symbolic::ParameterGenerator;
using ::symbolic::Pddl;
using ::symbolic::Proposition;
using ::symbolic::SignedProposition;

using AxiomApplicationFunction =
    std::function<std::optional<std::vector<Object>>(
        const std::vector<Object>&)>;

/**
 * Prepares list of possible arguments given axiom parameters.
 */
std::vector<std::vector<Object>> PrepareArguments(
    const Pddl& pddl, const ParameterGenerator& param_gen,
    const Formula& preconditions, const std::vector<Object>& parameters) {
  std::vector<std::vector<Object>> arguments;
  for (const std::vector<Object>& args : param_gen) {
    const std::optional<DisjunctiveFormula> dnf =
        DisjunctiveFormula::Create(pddl, preconditions, parameters, args);
    if (!dnf.has_value()) continue;
    arguments.push_back(args);
  }
  return arguments;
}

/**
 * Converts the axiom formula into a readable string.
 */
std::string StringifyFormula(const Pddl& pddl, const Formula& preconditions,
                             const VAL::effect_lists* postconditions,
                             const std::vector<Object>& parameters) {
  std::stringstream ss;
  try {
    ss << *DisjunctiveFormula::Create(pddl, preconditions, parameters,
                                      parameters);
  } catch (...) {
    ss << "false";
  }
  ss << " => ";
  try {
    ss << *DisjunctiveFormula::Create(pddl, postconditions, parameters,
                                      parameters);
  } catch (...) {
    ss << "false";
  }
  return ss.str();
}

/**
 * Extracts the predicate name used in the context.
 */
SignedProposition ExtractContextPredicate(const Pddl& pddl,
                                          const Formula& preconditions) {
  const VAL::goal* goal = preconditions.symbol();

  // Try simple predicate.
  bool is_pos = true;
  const auto* simple_goal = dynamic_cast<const VAL::simple_goal*>(goal);

  // Try negation of simple predicate.
  if (simple_goal == nullptr) {
    const auto* neg_goal = dynamic_cast<const VAL::neg_goal*>(goal);
    if (neg_goal != nullptr) {
      is_pos = false;
      simple_goal = dynamic_cast<const VAL::simple_goal*>(neg_goal->getGoal());
    }
  }

  // Context is more complex.
  if (simple_goal == nullptr) {
    std::cerr << "Axiom::Axiom(): Context must be a simple positive or "
                 "negative predicate."
              << std::endl;
  }

  std::string name_predicate = simple_goal->getProp()->head->getName();
  std::vector<Object> args =
      Object::CreateList(pddl, simple_goal->getProp()->args);

  return SignedProposition(name_predicate, std::move(args), is_pos);
}

}  // namespace

namespace symbolic {

/**
 * Axiom constructor.
 */
Axiom::Axiom(const Pddl& pddl, const VAL::operator_* symbol)
    : Action(pddl, symbol),
      arguments_(PrepareArguments(pddl, parameter_generator(), preconditions(),
                                  parameters())),
      context_(ExtractContextPredicate(pddl, preconditions())),
      formula_(StringifyFormula(pddl, preconditions(), postconditions(),
                                parameters())) {}

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
  return std::all_of(
      arguments_.begin(), arguments_.end(),
      [state, is_changed, this](const std::vector<Object>& args) {
        // Test if context is valid
        const std::optional<bool> is_valid = IsValid(*state, args);
        if (!is_valid || !*is_valid) return true;

        // Test if implies is valid (state shouldn't change)
        try {
          const int changed = Action::Apply(args, state);

          // Return false if proposition was negated.
          if (changed > 1) return false;

          *is_changed |= changed > 0;
        } catch (const std::exception& e) {
          // std::cerr << e.what() << std::endl;
          return false;
        }
        return true;
      });
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

bool Axiom::IsConsistent(const std::vector<std::shared_ptr<Axiom>>& axioms,
                         const PartialState& state) {
  PartialState test_state = state;

  // Keep testing axioms until state doesn't change for a full iteration
  bool is_changed = true;
  while (is_changed) {
    is_changed = false;
    for (const std::shared_ptr<Axiom>& axiom : axioms) {
      if (!axiom->IsConsistent(&test_state, &is_changed)) return false;
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

std::optional<AxiomApplicationFunction> Axiom::CreateApplicationFunction(
    const std::vector<Object>& action_params,
    const std::vector<Object>& action_prop_params,
    const std::vector<Object>& axiom_params,
    const std::vector<Object>& axiom_prop_params) {
  const size_t num_prop_params = action_prop_params.size();
  assert(num_prop_params == axiom_prop_params.size());

  // Map action prop parameter index to action parameter index.
  std::vector<std::optional<size_t>> idx_action_params(num_prop_params);
  for (size_t i = 0; i < num_prop_params; i++) {
    for (size_t j = 0; j < action_params.size(); j++) {
      if (action_prop_params[i] == action_params[j]) {
        idx_action_params[i] = j;
        break;
      }
    }
  }

  // Map axiom prop parameter index to axiom parameter index.
  std::vector<std::optional<size_t>> idx_axiom_params(num_prop_params);
  for (size_t i = 0; i < num_prop_params; i++) {
    for (size_t j = 0; j < axiom_params.size(); j++) {
      if (axiom_prop_params[i] == axiom_params[j]) {
        idx_axiom_params[i] = j;
        break;
      }
    }
  }

  // Check if axiom context will never be valid.
  for (size_t i = 0; i < num_prop_params; i++) {
    const std::optional<size_t>& idx_action_param = idx_action_params[i];
    const std::optional<size_t>& idx_axiom_param = idx_axiom_params[i];
    if (idx_action_param.has_value() || idx_axiom_param.has_value()) continue;

    const Object& action_prop_param = action_prop_params[i];
    const Object& axiom_prop_param = axiom_prop_params[i];
    if (action_prop_param != axiom_prop_param) return {};
  }

  return [axiom_params, action_prop_params, axiom_prop_params,
          idx_action_params = std::move(idx_action_params),
          idx_axiom_params = std::move(idx_axiom_params)](
             const std::vector<Object>& action_args)
             -> std::optional<std::vector<Object>> {
    std::vector<Object> axiom_args = axiom_params;
    for (size_t i = 0; i < action_prop_params.size(); i++) {
      const Object& action_prop_param = action_prop_params[i];
      const Object& axiom_prop_param = axiom_prop_params[i];
      const std::optional<size_t>& idx_action_param = idx_action_params[i];
      const std::optional<size_t>& idx_axiom_param = idx_axiom_params[i];

      const Object& action_prop_arg = idx_action_param.has_value()
                                          ? action_args[*idx_action_param]
                                          : action_prop_param;

      if (idx_axiom_param.has_value()) {
        Object& axiom_arg = axiom_args[*idx_axiom_param];
        axiom_arg = action_prop_arg;
      } else if (axiom_prop_param != action_prop_arg) {
        // Action prop not consistent with axiom context.
        return {};
      }
    }
    return axiom_args;
  };
}
static std::function<
    std::vector<std::vector<Object>>(const std::vector<Object>&)>
CreateApplicationFunction(const std::vector<Object>& action_params,
                          const std::vector<Object>& prop_params);

}  // namespace symbolic
