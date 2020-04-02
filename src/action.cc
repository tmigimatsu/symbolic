/**
 * actions.cc
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#include "symbolic/action.h"

#include <exception>  // std::runtime_error
#include <cassert>    // assert
#include <map>        // std::map
#include <sstream>    // std::stringstream

#include "symbolic/pddl.h"
#include "symbolic/utils/parameter_generator.h"

namespace {

using ::symbolic::Object;
using ::symbolic::Proposition;
using ::symbolic::Pddl;

using ActionFunction = std::function<std::set<Proposition>(const std::set<Proposition>&,
                                                           const std::vector<Object>&)>;

using EffectsFunction = std::function<void(const std::vector<Object>&, std::set<Proposition>*)>;

using ApplicationFunction = std::function<std::vector<Object>(const std::vector<Object>&)>;

EffectsFunction CreateEffectsFunction(const Pddl& pddl, const VAL::effect_lists* effects,
                                      const std::vector<Object>& parameters);

EffectsFunction CreateForall(const Pddl& pddl, const VAL::forall_effect* effect,
                             const std::vector<Object>& parameters) {
  // Create forall parameters
  std::vector<Object> forall_params = parameters;
  const std::vector<Object> types = symbolic::ConvertObjects(effect->getVarsList());
  forall_params.insert(forall_params.end(), types.begin(), types.end());
  const EffectsFunction ForallEffects = CreateEffectsFunction(pddl, effect->getEffects(), forall_params);

  return [&pddl, types = std::move(types),
          ForallEffects = std::move(ForallEffects)](const std::vector<Object>& arguments,
                                                    std::set<Proposition>* state) {
    // Loop over forall arguments
    symbolic::ParameterGenerator gen(pddl.object_map(), types);
    for (const std::vector<Object>& forall_objs : gen) {
      // Create forall arguments
      std::vector<Object> forall_args = arguments;
      forall_args.insert(forall_args.end(), forall_objs.begin(), forall_objs.end());

      ForallEffects(forall_args, state);
    }
  };
}

EffectsFunction CreateAdd(const Pddl& pddl, const VAL::simple_effect* effect,
                          const std::vector<Object>& parameters) {
  // Prepare effect argument application functions
  const std::vector<Object> effect_params = symbolic::ConvertObjects(effect->prop->args);
  ApplicationFunction Apply = symbolic::CreateApplicationFunction(parameters, effect_params);

  return [name_predicate = effect->prop->head->getName(),
          Apply = std::move(Apply)](const std::vector<Object>& arguments,
                                    std::set<Proposition>* state) {
    state->emplace(name_predicate, Apply(arguments));
  };
}

EffectsFunction CreateDel(const Pddl& pddl, const VAL::simple_effect* effect,
                          const std::vector<Object>& parameters) {
  // Prepare effect argument application functions
  const std::vector<Object> effect_params = symbolic::ConvertObjects(effect->prop->args);
  ApplicationFunction Apply = symbolic::CreateApplicationFunction(parameters, effect_params);

  return [name_predicate = effect->prop->head->getName(),
          Apply = std::move(Apply)](const std::vector<Object>& arguments,
                                    std::set<Proposition>* state) {
    state->erase(Proposition(name_predicate, Apply(arguments)));
  };
}

EffectsFunction CreateCond(const Pddl& pddl, const VAL::cond_effect* effect,
                           const std::vector<Object>& parameters) {
  const symbolic::Formula Condition(pddl, effect->getCondition(), parameters);
  const EffectsFunction CondEffects = CreateEffectsFunction(pddl, effect->getEffects(), parameters);
  return [Condition = std::move(Condition),
          CondEffects = std::move(CondEffects)](const std::vector<Object>& arguments,
                                                std::set<Proposition>* state) {
    // TODO: Condition might return different results depending on ordering of
    // other effects since state is modified in place.
    if (Condition(*state, arguments)) {
      CondEffects(arguments, state);
    }
  };
}

EffectsFunction CreateEffectsFunction(const Pddl& pddl, const VAL::effect_lists* effects,
                                      const std::vector<Object>& parameters) {
  std::vector<EffectsFunction> effect_functions;
  // Forall effects
  for (const VAL::forall_effect* effect : effects->forall_effects) {
    effect_functions.emplace_back(CreateForall(pddl, effect, parameters));
  }

  // Add effects
  for (const VAL::simple_effect* effect : effects->add_effects) {
    effect_functions.emplace_back(CreateAdd(pddl, effect, parameters));
  }

  // Del effects
  for (const VAL::simple_effect* effect : effects->del_effects) {
    effect_functions.emplace_back(CreateDel(pddl, effect, parameters));
  }

  // Cond effects
  for (const VAL::cond_effect* effect : effects->cond_effects) {
    effect_functions.emplace_back(CreateCond(pddl, effect, parameters));
  }

  return [effect_functions = std::move(effect_functions)](const std::vector<Object>& arguments,
                                                          std::set<Proposition>* state) {
    for (const EffectsFunction& Effect : effect_functions) {
      Effect(arguments, state);
    }
  };
}

const VAL::operator_* GetSymbol(const Pddl& pddl, const std::string& name_action) {
  assert(pddl.domain().ops != nullptr);
  for (const VAL::operator_* op : *pddl.domain().ops) {
    assert(op != nullptr && op->name != nullptr);
    if (op->name->getName() == name_action) return op;
  }
  throw std::runtime_error("Action::Action(): Could not find action symbol " + name_action + ".");
  return nullptr;
}

}  // namespace

namespace symbolic {

Action::Action(const Pddl& pddl, const VAL::operator_* symbol)
    : symbol_(symbol),
      name_(symbol_->name->getName()),
      parameters_(ConvertObjects(symbol_->parameters)),
      Preconditions_(pddl, symbol_->precondition, parameters_),
      Apply_(CreateEffectsFunction(pddl, symbol_->effects, parameters_)) {}

Action::Action(const Pddl& pddl, const std::string& action_call)
    : symbol_(GetSymbol(pddl, ParseHead(action_call))),
      name_(symbol_->name->getName()),
      parameters_(ConvertObjects(symbol_->parameters)),
      Preconditions_(pddl, symbol_->precondition, parameters_),
      Apply_(CreateEffectsFunction(pddl, symbol_->effects, parameters_)) {}

std::set<Proposition> Action::Apply(const std::set<Proposition>& state,
                                    const std::vector<Object>& arguments) const {
  std::set<Proposition> next_state(state);
  Apply_(arguments, &next_state);
  return next_state;
}

std::string Action::to_string() const {
  std::stringstream ss;
  ss << *this;
  return ss.str();
}

std::string Action::to_string(const std::vector<Object>& arguments) const {
  std::stringstream ss;
  ss << name_ << "(";
  std::string separator;
  for (const Object& arg : arguments) {
    ss << separator << arg;
    if (separator.empty()) separator = ", ";
  }
  ss << ")";
  return ss.str();
}

std::pair<Action, std::vector<Object>> ParseAction(const Pddl& pddl, const std::string& action_call) {
  return std::make_pair<Action, std::vector<Object>>(Action(pddl, action_call),
                                                     ParseArguments(pddl, action_call));
}

std::ostream& operator<<(std::ostream& os, const Action& action) {
  os << action.name() << "(";
  std::string separator;
  for (const Object& param : action.parameters()) {
    os << separator << param;
    if (separator.empty()) separator = ", ";
  }
  os << ")";
  return os;
}

}  // namespace symbolic
