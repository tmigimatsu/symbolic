/**
 * formula.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: March 20, 2020
 * Authors: Toki Migimatsu
 */

#include "symbolic/formula.h"

#include <VAL/ptree.h>

#include <cassert>        // assert
#include <exception>      // std::runtime_error
#include <sstream>        // std::stringstream
#include <unordered_map>  // std::unordered_map
#include <utility>        // std::move

#include "symbolic/pddl.h"

namespace {

using ::symbolic::Formula;
using ::symbolic::Object;
using ::symbolic::ParameterGenerator;
using ::symbolic::PartialState;
using ::symbolic::Pddl;
using ::symbolic::Proposition;

template <typename T>
using FormulaFunction =
    std::function<bool(const T& state, const std::vector<Object>& arguments)>;

using ApplicationFunction =
    std::function<std::vector<Object>(const std::vector<Object>&)>;

template <typename T>
using NamedFormulaFunction = std::pair<FormulaFunction<T>, std::string>;

template <typename T>
NamedFormulaFunction<T> CreateFormula(const Pddl& pddl, const VAL::goal* symbol,
                                      const std::vector<Object>& parameters);

template <typename T>
NamedFormulaFunction<T> CreateProposition(
    const Pddl& pddl, const VAL::simple_goal* symbol,
    const std::vector<Object>& parameters) {
  const VAL::proposition* prop = symbol->getProp();
  std::string name_predicate = prop->head->getName();
  const std::vector<Object> prop_params = Object::CreateList(pddl, prop->args);
  ApplicationFunction Apply =
      Formula::CreateApplicationFunction(parameters, prop_params);

  if (name_predicate == "=") {
    FormulaFunction<T> F = [Apply = std::move(Apply)](
                               // NOLINTNEXTLINE(misc-unused-parameters)
                               const T& state,
                               const std::vector<Object>& arguments) -> bool {
      const std::vector<Object> prop_args = Apply(arguments);
      assert(prop_args.size() == 2);
      return prop_args[0] == prop_args[1];
    };

    return {std::move(F), Proposition(name_predicate, prop_params).to_string()};
  }
  if (pddl.object_map().find(name_predicate) != pddl.object_map().end()) {
    // Predicate is a type. If it isn't in the object map, then no objects of
    // that type exist, and the proposition will be treated as a normal one
    // (and will always be false since the state will never contain it).
    FormulaFunction<T> F = [name_predicate, Apply = std::move(Apply)](
                               // NOLINTNEXTLINE(misc-unused-parameters)
                               const T& state,
                               const std::vector<Object>& arguments) -> bool {
      const std::vector<Object> prop_args = Apply(arguments);
      assert(prop_args.size() == 1);
      return prop_args[0].type().IsSubtype(name_predicate);
    };

    return {std::move(F), Proposition(name_predicate, prop_params).to_string()};
  }

  FormulaFunction<T> F = [name_predicate, Apply = std::move(Apply)](
                             const T& state,
                             const std::vector<Object>& arguments) -> bool {
    Proposition P(name_predicate, Apply(arguments));
    return state.contains(P);
  };

  return {std::move(F), Proposition(name_predicate, prop_params).to_string()};
}

template <typename T>
NamedFormulaFunction<T> CreateConjunction(
    const Pddl& pddl, const VAL::conj_goal* symbol,
    const std::vector<Object>& parameters) {
  std::stringstream ss("(");
  std::string delim;

  std::vector<FormulaFunction<T>> subformulas;
  const VAL::goal_list* goals = symbol->getGoals();
  subformulas.reserve(goals->size());
  for (const VAL::goal* goal : *goals) {
    NamedFormulaFunction<T> P_str = CreateFormula<T>(pddl, goal, parameters);
    subformulas.push_back(std::move(P_str.first));

    ss << delim << P_str.second;
    if (delim.empty()) delim = " && ";
  }
  ss << ")";

  FormulaFunction<T> F = [subformulas = std::move(subformulas)](
                             const T& state,
                             const std::vector<Object>& arguments) -> bool {
    for (const FormulaFunction<T>& P : subformulas) {
      if (!P(state, arguments)) return false;
    }
    return true;
  };

  return {std::move(F), ss.str()};
}

template <typename T>
NamedFormulaFunction<T> CreateDisjunction(
    const Pddl& pddl, const VAL::disj_goal* symbol,
    const std::vector<Object>& parameters) {
  std::stringstream ss("(");
  std::string delim;

  std::vector<FormulaFunction<T>> subformulas;
  const VAL::goal_list* goals = symbol->getGoals();
  subformulas.reserve(goals->size());
  for (const VAL::goal* goal : *goals) {
    NamedFormulaFunction<T> P_str = CreateFormula<T>(pddl, goal, parameters);
    subformulas.push_back(std::move(P_str.first));

    ss << delim << P_str.second;
    if (delim.empty()) delim = " || ";
  }
  ss << ")";

  FormulaFunction<T> F = [subformulas = std::move(subformulas)](
                             const T& state,
                             const std::vector<Object>& arguments) -> bool {
    for (const FormulaFunction<T>& P : subformulas) {
      if (P(state, arguments)) return true;
    }
    return false;
  };

  return {std::move(F), ss.str()};
}

template <typename T>
NamedFormulaFunction<T> CreateNegation(const Pddl& pddl,
                                       const VAL::neg_goal* symbol,
                                       const std::vector<Object>& parameters) {
  const VAL::goal* goal = symbol->getGoal();
  NamedFormulaFunction<T> P_str = CreateFormula<T>(pddl, goal, parameters);

  FormulaFunction<T> F = [P = std::move(P_str.first)](
                             const T& state,
                             const std::vector<Object>& arguments) -> bool {
    // Negate positive formula
    return !P(state, arguments);
  };

  return {std::move(F), "!" + P_str.second};
}
template <>
NamedFormulaFunction<PartialState> CreateNegation(
    const Pddl& pddl, const VAL::neg_goal* symbol,
    const std::vector<Object>& parameters) {
  const VAL::goal* goal = symbol->getGoal();

  // Check if goal is a simple proposition.
  const VAL::simple_goal* simple_goal =
      dynamic_cast<const VAL::simple_goal*>(goal);
  if (simple_goal != nullptr) {
    const VAL::proposition* prop = simple_goal->getProp();
    std::string name_predicate = prop->head->getName();
    if (name_predicate != "=" &&
        pddl.object_map().find(name_predicate) == pddl.object_map().end()) {
      // Proposition is a simple goal, so evaluate its negation.

      const std::vector<Object> prop_params =
          Object::CreateList(pddl, prop->args);
      ApplicationFunction Apply =
          Formula::CreateApplicationFunction(parameters, prop_params);

      FormulaFunction<PartialState> F =
          [name_predicate, Apply = std::move(Apply)](
              const PartialState& state,
              const std::vector<Object>& arguments) -> bool {
        Proposition P(name_predicate, Apply(arguments));
        return state.does_not_contain(P);
      };

      return {std::move(F),
              "!" + Proposition(name_predicate, prop_params).to_string()};
    }
    // Otherwise proposition is an = or type, so evaluate as normal.
  }

  NamedFormulaFunction<PartialState> P_str =
      CreateFormula<PartialState>(pddl, goal, parameters);

  FormulaFunction<PartialState> F =
      [P = std::move(P_str.first)](
          const PartialState& state,
          const std::vector<Object>& arguments) -> bool {
    // Negate positive formula
    return !P(state, arguments);
  };

  return {std::move(F), "!" + P_str.second};
}

template <typename T>
NamedFormulaFunction<T> CreateForall(const Pddl& pddl,
                                     const VAL::qfied_goal* symbol,
                                     const std::vector<Object>& parameters) {
  // Create forall parameters
  std::vector<Object> forall_params = parameters;
  std::vector<Object> types = Object::CreateList(pddl, symbol->getVars());
  forall_params.insert(forall_params.end(), types.begin(), types.end());

  const VAL::goal* goal = symbol->getGoal();
  NamedFormulaFunction<T> P_str = CreateFormula<T>(pddl, goal, forall_params);

  FormulaFunction<T> F = [gen = ParameterGenerator(pddl.object_map(), types),
                          types = std::move(types), P = std::move(P_str.first)](
                             const T& state,
                             const std::vector<Object>& arguments) -> bool {
    // Loop over forall arguments
    for (const std::vector<Object>& forall_objs : gen) {
      // Create forall arguments
      std::vector<Object> forall_args = arguments;
      forall_args.insert(forall_args.end(), forall_objs.begin(),
                         forall_objs.end());

      if (!P(state, forall_args)) return false;
    }
    return true;
  };

  std::stringstream ss("(forall ");
  std::string delim;
  for (const Object& param : parameters) {
    ss << delim << param;
    if (delim.empty()) delim = ", ";
  }
  ss << " =>" << std::endl << P_str.second << std::endl << ")";

  return {std::move(F), ss.str()};
}

template <typename T>
NamedFormulaFunction<T> CreateExists(const Pddl& pddl,
                                     const VAL::qfied_goal* symbol,
                                     const std::vector<Object>& parameters) {
  // Create exists parameters
  std::vector<Object> exists_params = parameters;
  std::vector<Object> types = Object::CreateList(pddl, symbol->getVars());
  exists_params.insert(exists_params.end(), types.begin(), types.end());

  const VAL::goal* goal = symbol->getGoal();
  NamedFormulaFunction<T> P_str = CreateFormula<T>(pddl, goal, exists_params);

  FormulaFunction<T> F = [gen = ParameterGenerator(pddl.object_map(), types),
                          types = std::move(types), P = std::move(P_str.first)](
                             const T& state,
                             const std::vector<Object>& arguments) -> bool {
    // Loop over exists arguments
    for (const std::vector<Object>& exists_objs : gen) {
      // Create exists arguments
      std::vector<Object> exists_args = arguments;
      exists_args.insert(exists_args.end(), exists_objs.begin(),
                         exists_objs.end());

      if (P(state, exists_args)) return true;
    }
    return false;
  };

  std::stringstream ss("(exists ");
  std::string delim;
  for (const Object& param : parameters) {
    ss << delim << param;
    if (delim.empty()) delim = ", ";
  }
  ss << " =>" << std::endl << P_str.second << std::endl << ")";

  return {std::move(F), ss.str()};
}

template <typename T>
NamedFormulaFunction<T> CreateFormula(const Pddl& pddl, const VAL::goal* symbol,
                                      const std::vector<Object>& parameters) {
  // Proposition
  const auto* simple_goal = dynamic_cast<const VAL::simple_goal*>(symbol);
  if (simple_goal != nullptr) {
    return CreateProposition<T>(pddl, simple_goal, parameters);
  }

  // Conjunction
  const auto* conj_goal = dynamic_cast<const VAL::conj_goal*>(symbol);
  if (conj_goal != nullptr) {
    return CreateConjunction<T>(pddl, conj_goal, parameters);
  }

  // Disjunction
  const auto* disj_goal = dynamic_cast<const VAL::disj_goal*>(symbol);
  if (disj_goal != nullptr) {
    return CreateDisjunction<T>(pddl, disj_goal, parameters);
  }

  // Negation
  const auto* neg_goal = dynamic_cast<const VAL::neg_goal*>(symbol);
  if (neg_goal != nullptr) {
    return CreateNegation<T>(pddl, neg_goal, parameters);
  }

  // Forall
  const auto* qfied_goal = dynamic_cast<const VAL::qfied_goal*>(symbol);
  if (qfied_goal != nullptr) {
    switch (qfied_goal->getQuantifier()) {
      case VAL::quantifier::E_FORALL:
        return CreateForall<T>(pddl, qfied_goal, parameters);
      case VAL::quantifier::E_EXISTS:
        return CreateExists<T>(pddl, qfied_goal, parameters);
    }
  }

  throw std::runtime_error("GetFormula(): Goal type not implemented.");
}

}  // namespace

namespace symbolic {

Formula::Formula(const Pddl& pddl, const VAL::goal* symbol,
                 const std::vector<Object>& parameters)
    : symbol_(symbol),
      P_(CreateFormula<State>(pddl, symbol, parameters).first) {
  NamedFormulaFunction<PartialState> pp_str =
      CreateFormula<PartialState>(pddl, symbol, parameters);
  PP_ = pp_str.first;
  str_formula_ = pp_str.second;
}

std::optional<bool> Formula::operator()(
    const PartialState& state, const std::vector<Object>& arguments) const {
  try {
    return PP_(state, arguments);
  } catch (const PartialState::UnknownEvaluation& e) {
    return {};
  }
};

std::optional<bool> Formula::operator()(const PartialState& state) const {
  try {
    return PP_(state, {});
  } catch (const PartialState::UnknownEvaluation& e) {
    return {};
  }
};

ostream& operator<<(ostream& os, const Formula& F) {
  os << F.to_string();
  return os;
}

ApplicationFunction Formula::CreateApplicationFunction(
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
