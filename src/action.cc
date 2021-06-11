/**
 * action.cc
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#include "symbolic/action.h"

#include <VAL/ptree.h>

#include <algorithm>  // std::max
#include <cassert>    // assert
#include <exception>  // std::runtime_error, std::invalid_argument
#include <sstream>    // std::stringstream

#include "symbolic/pddl.h"
#include "symbolic/utils/parameter_generator.h"

namespace {

using ::symbolic::Axiom;
using ::symbolic::Formula;
using ::symbolic::Object;
using ::symbolic::ParameterGenerator;
using ::symbolic::Pddl;
using ::symbolic::Proposition;
using ::symbolic::PropositionRef;
using ::symbolic::SignedProposition;
using ::symbolic::State;

template <typename T>
using EffectsFunction = std::function<int(const std::vector<Object>&, T*)>;

using ApplicationFunction =
    std::function<const std::vector<Object>&(const std::vector<Object>&)>;

using AxiomApplicationFunction =
    std::function<const std::vector<Object>*(const std::vector<Object>&)>;

template <typename T>
EffectsFunction<T> CreateEffectsFunction(const Pddl& pddl,
                                         const VAL::effect_lists* effects,
                                         const std::vector<Object>& parameters);

template <typename T>
EffectsFunction<T> CreateForall(const Pddl& pddl,
                                const VAL::forall_effect* effect,
                                const std::vector<Object>& parameters) {
  // Create forall parameters
  std::vector<Object> forall_params = parameters;
  const std::vector<Object> types =
      Object::CreateList(pddl, effect->getVarsList());
  forall_params.insert(forall_params.end(), types.begin(), types.end());
  EffectsFunction<T> ForallEffects =
      CreateEffectsFunction<T>(pddl, effect->getEffects(), forall_params);

  return [gen = ParameterGenerator(pddl, types),
          ForallEffects = std::move(ForallEffects)](
             const std::vector<Object>& arguments, T* state) -> int {
    // Loop over forall arguments
    int is_state_changed = 0;
    for (const std::vector<Object>& forall_objs : gen) {
      // Create forall arguments
      std::vector<Object> forall_args = arguments;
      forall_args.insert(forall_args.end(), forall_objs.begin(),
                         forall_objs.end());

      is_state_changed =
          std::max(is_state_changed, ForallEffects(forall_args, state));
    }
    return is_state_changed;
  };
}

template <typename T>
EffectsFunction<T> CreateAdd(const Pddl& pddl, const VAL::simple_effect* effect,
                             const std::vector<Object>& parameters) {
  // Prepare effect argument application functions
  const std::vector<Object> effect_params =
      Object::CreateList(pddl, effect->prop->args);
  ApplicationFunction Apply =
      Formula::CreateApplicationFunction(parameters, effect_params);

  const std::string& name_predicate = effect->prop->head->getNameRef();
  if (name_predicate == "=") {
    // Equality predicate
    return [Apply = std::move(Apply)](const std::vector<Object>& arguments,
                                      // NOLINTNEXTLINE(misc-unused-parameters)
                                      T* state) -> int {
      const std::vector<Object>& prop_args = Apply(arguments);
      assert(prop_args.size() == 2);
      if (prop_args[0] != prop_args[1]) {
        std::stringstream ss;
        ss << "Action::Apply(): Cannot add effect: "
           << Proposition("=", prop_args) << ".";
        throw std::runtime_error(ss.str());
      }
      return false;
    };
  }
  if (pddl.object_map().count(name_predicate) > 0) {
    // Type predicate
    return [&name_predicate, Apply = std::move(Apply)](
               // NOLINTNEXTLINE(misc-unused-parameters)
               const std::vector<Object>& arguments, T* state) -> int {
      const std::vector<Object>& prop_args = Apply(arguments);
      assert(prop_args.size() == 1);
      if (!prop_args[0].type().IsSubtype(name_predicate)) {
        std::stringstream ss;
        ss << "Action::Apply(): Cannot add effect: "
           << Proposition(name_predicate, prop_args) << ".";
        throw std::runtime_error(ss.str());
      }
      return false;
    };
  }

  // Prepare axioms.
  const std::string axiom_context =
      SignedProposition::Sign(true) + name_predicate;
  std::vector<std::pair<std::weak_ptr<Axiom>, AxiomApplicationFunction>> axioms;
  if (pddl.axiom_map().count(axiom_context) > 0) {
    for (const std::weak_ptr<Axiom>& ptr_axiom :
         pddl.axiom_map().at(axiom_context)) {
      const std::shared_ptr<Axiom> axiom = ptr_axiom.lock();
      std::optional<AxiomApplicationFunction> AxiomApply =
          Axiom::CreateApplicationFunction(parameters, effect_params,
                                           axiom->parameters(),
                                           axiom->context().arguments());
      if (!AxiomApply.has_value()) continue;

      axioms.emplace_back(axiom, std::move(*AxiomApply));
    }
  }

  // Add normal predicate
  const size_t predicate_hash = std::hash<std::string>{}(name_predicate);
  return [ptr_name_predicate = &name_predicate, predicate_hash,
          Apply = std::move(Apply), axioms = std::move(axioms)](
             const std::vector<Object>& arguments, T* state) -> int {
    int status = state->insert(
        PropositionRef(ptr_name_predicate, &Apply(arguments), predicate_hash));
    // Return early to avoid infinite loop of axiom application.
    if (status == 0) return status;

    // Apply axioms.
    for (const auto& axiom_apply : axioms) {
      const Axiom& axiom = *axiom_apply.first.lock();
      const AxiomApplicationFunction& AxiomApply = axiom_apply.second;
      const std::vector<Object>* axiom_args = AxiomApply(arguments);

      // std::cout << axiom << std::endl;
      if (axiom_args == nullptr) continue;

      // std::cout << "+" << *name_predicate << ": " << *axiom_args <<
      // std::endl; std::cout << "[" << *state << std::endl;
      axiom.Action::Apply(*axiom_args, state);
      // std::cout << "]" << *state << std::endl << std::endl;
    }
    return status;
  };
}

template <typename T>
EffectsFunction<T> CreateDel(const Pddl& pddl, const VAL::simple_effect* effect,
                             const std::vector<Object>& parameters) {
  // Prepare effect argument application functions
  const std::vector<Object> effect_params =
      Object::CreateList(pddl, effect->prop->args);
  ApplicationFunction Apply =
      Formula::CreateApplicationFunction(parameters, effect_params);

  const std::string& name_predicate = effect->prop->head->getNameRef();
  if (name_predicate == "=") {
    // Equality predicate
    return [Apply = std::move(Apply)](const std::vector<Object>& arguments,
                                      // NOLINTNEXTLINE(misc-unused-parameters)
                                      T* state) -> int {
      const std::vector<Object>& prop_args = Apply(arguments);
      assert(prop_args.size() == 2);
      if (prop_args[0] == prop_args[1]) {
        std::stringstream ss;
        ss << "Action::Apply(): Cannot delete effect: "
           << Proposition("=", prop_args) << ".";
        throw std::runtime_error(ss.str());
      }
      return false;
    };
  }
  if (pddl.object_map().count(name_predicate) > 0) {
    // Type predicate
    return [&name_predicate, Apply = std::move(Apply)](
               // NOLINTNEXTLINE(misc-unused-parameters)
               const std::vector<Object>& arguments, T* state) -> int {
      const std::vector<Object>& prop_args = Apply(arguments);
      assert(prop_args.size() == 1);
      if (prop_args[0].type().IsSubtype(name_predicate)) {
        std::stringstream ss;
        ss << "Action::Apply(): Cannot delete effect: "
           << Proposition(name_predicate, prop_args) << ".";
        throw std::runtime_error(ss.str());
      }
      return false;
    };
  }

  // Prepare axioms.
  const std::string axiom_context =
      SignedProposition::Sign(false) + name_predicate;
  std::vector<std::pair<std::weak_ptr<Axiom>, AxiomApplicationFunction>> axioms;
  if (pddl.axiom_map().count(axiom_context) > 0) {
    for (const std::weak_ptr<Axiom>& ptr_axiom :
         pddl.axiom_map().at(axiom_context)) {
      const std::shared_ptr<Axiom> axiom = ptr_axiom.lock();
      std::optional<AxiomApplicationFunction> AxiomApply =
          Axiom::CreateApplicationFunction(parameters, effect_params,
                                           axiom->parameters(),
                                           axiom->context().arguments());
      if (!AxiomApply.has_value()) continue;

      axioms.emplace_back(axiom, std::move(*AxiomApply));
    }
  }

  // Remove normal predicate
  const size_t predicate_hash = std::hash<std::string>{}(name_predicate);
  return [ptr_name_predicate = &name_predicate, predicate_hash,
          Apply = std::move(Apply), axioms = std::move(axioms)](
             const std::vector<Object>& arguments, T* state) -> int {
    int status = state->erase(
        PropositionRef(ptr_name_predicate, &Apply(arguments), predicate_hash));
    // Return early to avoid infinite loop of axiom application.
    if (status == 0) return status;

    // Apply axioms.
    for (const auto& axiom_apply : axioms) {
      const Axiom& axiom = *axiom_apply.first.lock();
      const AxiomApplicationFunction& AxiomApply = axiom_apply.second;
      const std::vector<Object>* axiom_args = AxiomApply(arguments);

      // std::cout << axiom << std::endl;
      if (axiom_args == nullptr) continue;

      // std::cout << "-" << *name_predicate << ": " << *axiom_args <<
      // std::endl; std::cout << "[" << *state << std::endl;
      axiom.Action::Apply(*axiom_args, state);
      // std::cout << "]" << *state << std::endl << std::endl;
    }
    return status;
  };
}

bool EvaluateCondition(bool cond) { return cond; }

bool EvaluateCondition(const std::optional<bool>& cond) {
  return cond ? *cond : false;
}

template <typename T>
EffectsFunction<T> CreateCond(const Pddl& pddl, const VAL::cond_effect* effect,
                              const std::vector<Object>& parameters) {
  Formula Condition(pddl, effect->getCondition(), parameters);
  EffectsFunction<T> CondEffects =
      CreateEffectsFunction<T>(pddl, effect->getEffects(), parameters);
  return
      [Condition = std::move(Condition), CondEffects = std::move(CondEffects)](
          const std::vector<Object>& arguments, T* state) -> int {
        // TODO(tmigimatsu): Condition might return different results depending
        // on ordering of other effects since state is modified in place.
        if (EvaluateCondition(Condition(*state, arguments))) {
          return CondEffects(arguments, state);
        }
        return false;
      };
}

template <typename T>
EffectsFunction<T> CreateEffectsFunction(
    const Pddl& pddl, const VAL::effect_lists* effects,
    const std::vector<Object>& parameters) {
  std::vector<EffectsFunction<T>> effect_functions;
  // Forall effects
  for (const VAL::forall_effect* effect : effects->forall_effects) {
    effect_functions.emplace_back(CreateForall<T>(pddl, effect, parameters));
  }

  // Add effects
  for (const VAL::simple_effect* effect : effects->add_effects) {
    effect_functions.emplace_back(CreateAdd<T>(pddl, effect, parameters));
  }

  // Del effects
  for (const VAL::simple_effect* effect : effects->del_effects) {
    effect_functions.emplace_back(CreateDel<T>(pddl, effect, parameters));
  }

  // Cond effects
  for (const VAL::cond_effect* effect : effects->cond_effects) {
    effect_functions.emplace_back(CreateCond<T>(pddl, effect, parameters));
  }

  return [effect_functions = std::move(effect_functions)](
             const std::vector<Object>& arguments, T* state) -> int {
    int is_state_changed = 0;
    for (const EffectsFunction<T>& Effect : effect_functions) {
      is_state_changed = std::max(is_state_changed, Effect(arguments, state));
    }
    return is_state_changed;
  };
}

const VAL::operator_* GetSymbol(const Pddl& pddl,
                                const std::string& name_action) {
  assert(pddl.symbol()->the_domain->ops != nullptr);
  for (const VAL::operator_* op : *pddl.symbol()->the_domain->ops) {
    assert(op != nullptr && op->name != nullptr);
    if (op->name->getNameRef() == name_action) return op;
  }
  throw std::runtime_error("Action::Action(): Could not find action symbol " +
                           name_action + ".");
  return nullptr;
}

}  // namespace

namespace symbolic {

Action::Action(const Pddl& pddl, const VAL::operator_* symbol)
    : symbol_(symbol),
      pddl_(&pddl),
      name_(symbol_->name->getNameRef()),
      parameters_(Object::CreateList(pddl, symbol_->parameters)),
      param_gen_(pddl, parameters_),
      Preconditions_(pddl, symbol_->precondition, parameters_),
      Apply_(CreateEffectsFunction<State>(pddl, symbol_->effects, parameters_)),
      ApplyPartial_(CreateEffectsFunction<PartialState>(pddl, symbol_->effects,
                                                        parameters_)) {}

Action::Action(const Pddl& pddl, const std::string& action_call)
    : Action(pddl, GetSymbol(pddl, Proposition::ParseHead(action_call))) {}

State Action::Apply(const State& state,
                    const std::vector<Object>& arguments) const {
  State next_state(state);
  Apply_(arguments, &next_state);
  return next_state;
}

PartialState Action::Apply(const PartialState& state,
                           const std::vector<Object>& arguments) const {
  PartialState next_state(state);
  ApplyPartial_(arguments, &next_state);
  return next_state;
}

const VAL::effect_lists* Action::postconditions() const {
  return symbol_->effects;
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

std::pair<Action, std::vector<Object>> Action::Parse(
    const Pddl& pddl, const std::string& action_call) {
  auto aa = std::make_pair(Action(pddl, action_call),
                           Object::ParseArguments(pddl, action_call));
  const Action& action = aa.first;
  const std::vector<Object>& args = aa.second;

  // Check number of arguments
  if (action.parameters().size() != args.size()) {
    std::stringstream ss;
    ss << "symbolic::ParseAction(): action " << action << " requires "
       << action.parameters().size() << " arguments but received "
       << args.size() << ": " << action_call << ".";
    throw std::invalid_argument(ss.str());
  }

  // Check argument types
  for (size_t i = 0; i < action.parameters().size(); i++) {
    const Object& param = action.parameters()[i];
    const Object& arg = args[i];
    if (!arg.type().IsSubtype(param.type())) {
      std::stringstream ss;
      ss << "symbolic::ParseAction(): action " << action
         << " requires parameter " << param << " to be of type " << param.type()
         << " but received " << arg << " with type " << arg << ": "
         << action_call << ".";
      throw std::invalid_argument(ss.str());
    }
  }

  return aa;
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
