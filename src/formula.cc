/**
 * formula.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: March 20, 2020
 * Authors: Toki Migimatsu
 */

#include "symbolic/formula.h"

#include <cassert>        // assert
#include <exception>      // std::runtime_error
#include <unordered_map>  // std::unordered_map
#include <utility>        // std::move

#include "symbolic/pddl.h"

namespace {

using ::symbolic::Object;
using ::symbolic::Pddl;
using ::symbolic::Proposition;
using ::symbolic::State;

using FormulaFunction = std::function<bool(
    const State& state, const std::vector<Object>& arguments)>;

using ApplicationFunction =
    std::function<std::vector<Object>(const std::vector<Object>&)>;

FormulaFunction CreateFormula(const Pddl& pddl, const VAL::goal* symbol,
                              const std::vector<Object>& parameters);

FormulaFunction CreateProposition(const Pddl& pddl,
                                  const VAL::simple_goal* symbol,
                                  const std::vector<Object>& parameters) {
  const VAL::proposition* prop = symbol->getProp();
  std::string name_predicate = prop->head->getName();
  const std::vector<Object> prop_params =
      symbolic::ConvertObjects(pddl, prop->args);
  ApplicationFunction Apply =
      CreateApplicationFunction(parameters, prop_params);

  if (name_predicate == "=") {
    // NOLINTNEXTLINE(misc-unused-parameters)
    return [Apply = std::move(Apply)](const State& state,
                                      const std::vector<Object>& arguments) {
      const std::vector<Object> prop_args = Apply(arguments);
      assert(prop_args.size() == 2);
      return prop_args[0] == prop_args[1];
    };
  }
  if (pddl.object_map().find(name_predicate) != pddl.object_map().end()) {
    // Predicate is a type. If it isn't in the object map, then no objects of
    // that type exist, and the proposition will be treated as a normal one
    // (and will always be false since the state will never contain it).
    return [name_predicate, Apply = std::move(Apply)](
               // NOLINTNEXTLINE(misc-unused-parameters)
               const State& state, const std::vector<Object>& arguments) {
      const std::vector<Object> prop_args = Apply(arguments);
      assert(prop_args.size() == 1);
      return prop_args[0].type().IsSubtype(name_predicate);
    };
  }

  return [name_predicate, Apply = std::move(Apply)](
             const State& state, const std::vector<Object>& arguments) {
    Proposition P(name_predicate, Apply(arguments));
    return state.contains(P);
  };
}

FormulaFunction CreateConjunction(const Pddl& pddl,
                                  const VAL::conj_goal* symbol,
                                  const std::vector<Object>& parameters) {
  std::vector<FormulaFunction> subformulas;
  const VAL::goal_list* goals = symbol->getGoals();
  subformulas.reserve(goals->size());
  for (const VAL::goal* goal : *goals) {
    subformulas.push_back(CreateFormula(pddl, goal, parameters));
  }

  return [subformulas = std::move(subformulas)](
             const State& state, const std::vector<Object>& arguments) -> bool {
    for (const FormulaFunction& P : subformulas) {
      if (!P(state, arguments)) return false;
    }
    return true;
  };
}

FormulaFunction CreateDisjunction(const Pddl& pddl,
                                  const VAL::disj_goal* symbol,
                                  const std::vector<Object>& parameters) {
  std::vector<FormulaFunction> subformulas;
  const VAL::goal_list* goals = symbol->getGoals();
  subformulas.reserve(goals->size());
  for (const VAL::goal* goal : *goals) {
    subformulas.push_back(CreateFormula(pddl, goal, parameters));
  }

  return [subformulas = std::move(subformulas)](
             const State& state, const std::vector<Object>& arguments) -> bool {
    for (const FormulaFunction& P : subformulas) {
      if (P(state, arguments)) return true;
    }
    return false;
  };
}

FormulaFunction CreateNegation(const Pddl& pddl, const VAL::neg_goal* symbol,
                               const std::vector<Object>& parameters) {
  const VAL::goal* goal = symbol->getGoal();
  FormulaFunction P = CreateFormula(pddl, goal, parameters);
  return [P = std::move(P)](const State& state,
                            const std::vector<Object>& arguments) -> bool {
    // Negate positive formula
    return !P(state, arguments);
  };
}

FormulaFunction CreateForall(const Pddl& pddl, const VAL::qfied_goal* symbol,
                             const std::vector<Object>& parameters) {
  // Create forall parameters
  std::vector<Object> forall_params = parameters;
  std::vector<Object> types = symbolic::ConvertObjects(pddl, symbol->getVars());
  forall_params.insert(forall_params.end(), types.begin(), types.end());

  const VAL::goal* goal = symbol->getGoal();
  FormulaFunction P = CreateFormula(pddl, goal, forall_params);

  return [&pddl, types = std::move(types), P = std::move(P)](
             const State& state, const std::vector<Object>& arguments) -> bool {
    // Loop over forall arguments
    symbolic::ParameterGenerator gen(pddl.object_map(), types);
    for (const std::vector<Object>& forall_objs : gen) {
      // Create forall arguments
      std::vector<Object> forall_args = arguments;
      forall_args.insert(forall_args.end(), forall_objs.begin(),
                         forall_objs.end());

      if (!P(state, forall_args)) return false;
    }
    return true;
  };
}

FormulaFunction CreateExists(const Pddl& pddl, const VAL::qfied_goal* symbol,
                             const std::vector<Object>& parameters) {
  // Create exists parameters
  std::vector<Object> exists_params = parameters;
  std::vector<Object> types = symbolic::ConvertObjects(pddl, symbol->getVars());
  exists_params.insert(exists_params.end(), types.begin(), types.end());

  const VAL::goal* goal = symbol->getGoal();
  FormulaFunction P = CreateFormula(pddl, goal, exists_params);

  return [&pddl, types = std::move(types), P = std::move(P)](
             const State& state, const std::vector<Object>& arguments) -> bool {
    // Loop over exists arguments
    symbolic::ParameterGenerator gen(pddl.object_map(), types);
    for (const std::vector<Object>& exists_objs : gen) {
      // Create exists arguments
      std::vector<Object> exists_args = arguments;
      exists_args.insert(exists_args.end(), exists_objs.begin(),
                         exists_objs.end());

      if (P(state, exists_args)) return true;
    }
    return false;
  };
}

FormulaFunction CreateFormula(const Pddl& pddl, const VAL::goal* symbol,
                              const std::vector<Object>& parameters) {
  // Proposition
  const auto* simple_goal = dynamic_cast<const VAL::simple_goal*>(symbol);
  if (simple_goal != nullptr) {
    return CreateProposition(pddl, simple_goal, parameters);
  }

  // Conjunction
  const auto* conj_goal = dynamic_cast<const VAL::conj_goal*>(symbol);
  if (conj_goal != nullptr) {
    return CreateConjunction(pddl, conj_goal, parameters);
  }

  // Disjunction
  const auto* disj_goal = dynamic_cast<const VAL::disj_goal*>(symbol);
  if (disj_goal != nullptr) {
    return CreateDisjunction(pddl, disj_goal, parameters);
  }

  // Negation
  const auto* neg_goal = dynamic_cast<const VAL::neg_goal*>(symbol);
  if (neg_goal != nullptr) {
    return CreateNegation(pddl, neg_goal, parameters);
  }

  // Forall
  const auto* qfied_goal = dynamic_cast<const VAL::qfied_goal*>(symbol);
  if (qfied_goal != nullptr) {
    switch (qfied_goal->getQuantifier()) {
      case VAL::quantifier::E_FORALL:
        return CreateForall(pddl, qfied_goal, parameters);
      case VAL::quantifier::E_EXISTS:
        return CreateExists(pddl, qfied_goal, parameters);
    }
  }

  throw std::runtime_error("GetFormula(): Goal type not implemented.");
}

}  // namespace

namespace symbolic {

Formula::Formula(const Pddl& pddl, const VAL::goal* symbol,
                 const std::vector<Object>& parameters)
    : symbol_(symbol), P_(CreateFormula(pddl, symbol, parameters)) {}

ApplicationFunction CreateApplicationFunction(
    const std::vector<Object>& action_params,
    const std::vector<Object>& prop_params) {
  // Map action parameter index to vector of proposition parameter indices
  std::unordered_map<size_t, std::vector<size_t>> idx_params;
  for (size_t i = 0; i < prop_params.size(); i++) {
    for (size_t j = 0; j < action_params.size(); j++) {
      if (prop_params[i] == action_params[j]) {
        idx_params[j].push_back(i);
        break;
      }
    }
  }
  return [prop_params, idx_params = std::move(idx_params)](
             const std::vector<Object>& action_args) {
    // Replace proposition parameters with corresponding action arguments
    std::vector<Object> prop_args = prop_params;
    for (const auto& key_val : idx_params) {
      const size_t idx_action_arg = key_val.first;
      const std::vector<size_t>& idx_prop_params = key_val.second;
      for (size_t idx_prop_param : idx_prop_params) {
        prop_args[idx_prop_param] = action_args[idx_action_arg];
      }
    }
    return prop_args;
  };
}

}  // namespace symbolic
