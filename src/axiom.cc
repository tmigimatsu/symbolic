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

#include <exception>  // std::domain_error
#include <sstream>    // std::stringstream

#include "symbolic/normal_form.h"

namespace {

using ::symbolic::DisjunctiveFormula;
using ::symbolic::Formula;
using ::symbolic::Object;
using ::symbolic::ParameterGenerator;
using ::symbolic::Pddl;
using ::symbolic::SignedProposition;

using AxiomApplicationFunction =
    std::function<const std::vector<Object>*(const std::vector<Object>&)>;

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
    throw std::domain_error(
        "Axiom::Axiom(): Context must be a simple positive or negative "
        "predicate.");
  }

  const std::string& name_predicate =
      simple_goal->getProp()->head->getNameRef();
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

  // Check unmatched axiom prop param equal action param.
  std::vector<std::pair<size_t, size_t>> idx_params;
  auto axiom_args = std::make_shared<std::vector<Object>>(axiom_params);
  std::vector<std::pair<size_t, Object>> future_action_args;
  for (size_t idx_prop = 0; idx_prop < num_prop_params; idx_prop++) {
    // Check if axiom prop param has corresponding axiom param.
    const Object& axiom_prop_param = axiom_prop_params[idx_prop];
    size_t i = 0;
    for (; i < axiom_params.size(); i++) {
      if (axiom_params[i] == axiom_prop_param) break;
    }
    const bool is_axiom_prop_arg = i == axiom_params.size();

    // Check if action prop param has corresponding action param.
    const Object& action_prop_param = action_prop_params[idx_prop];
    size_t j = 0;
    for (; j < action_params.size(); j++) {
      if (action_params[j] == action_prop_param) break;
    }
    const bool is_action_prop_arg = j == action_params.size();

    if (is_axiom_prop_arg && is_action_prop_arg) {
      // Make sure axiom prop arg and action prop arg are equal.
      if (axiom_prop_param != action_prop_param) return {};
    } else if (is_axiom_prop_arg) {
      // Make sure axiom prop arg and future action prop arg are equal.
      future_action_args.emplace_back(j, axiom_prop_param);
    } else if (is_action_prop_arg) {
      // Instantiate axiom prop param with action prop arg.
      (*axiom_args)[i] = action_prop_param;
    } else {
      // Match axiom prop param and action prop param.
      idx_params.emplace_back(i, j);
    }
  }

  // TODO(tmigimatsu): shared_ptr makes this not thread-safe.
  return [ptr_axiom_args = std::move(axiom_args),
          idx_params = std::move(idx_params),
          future_action_args = std::move(future_action_args)](
             const std::vector<Object>& action_args)
             -> const std::vector<Object>* {
    // Check that action args match up with axiom context proposition.
    for (const std::pair<size_t, Object>& idx_argument : future_action_args) {
      const size_t idx_arg = idx_argument.first;
      const Object& expected_arg = idx_argument.second;
      if (action_args[idx_arg] != expected_arg) return nullptr;
    }

    // Assign axiom args to action args.
    std::vector<Object>& axiom_args = *ptr_axiom_args;
    for (const std::pair<size_t, size_t>& idx_axiom_action : idx_params) {
      const size_t idx_axiom = idx_axiom_action.first;
      const size_t idx_action = idx_axiom_action.second;
      axiom_args[idx_axiom] = action_args[idx_action];
    }
    return &axiom_args;
  };
}

}  // namespace symbolic
