/**
 * action.h
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_ACTION_H_
#define SYMBOLIC_ACTION_H_

#include <functional>  // std::function
#include <ostream>     // std::ostream
#include <string>      // std::string
#include <utility>     // std::pair
#include <vector>      // std::vector

#include "symbolic/formula.h"
#include "symbolic/object.h"
#include "symbolic/proposition.h"
#include "symbolic/state.h"
#include "symbolic/utils/parameter_generator.h"

namespace VAL {

class operator_;
class effect_lists;

}  // namespace VAL

namespace symbolic {

class Pddl;

class Action {
 public:
  Action() = default;

  Action(const Pddl& pddl, const VAL::operator_* symbol);

  // action_call can be action(params) or action name
  Action(const Pddl& pddl, const std::string& action_call);

  bool IsValid(const State& state, const std::vector<Object>& arguments) const {
    return Preconditions_(state, arguments);
  }

  std::optional<bool> IsValid(const PartialState& state,
                              const std::vector<Object>& arguments) const {
    return Preconditions_(state, arguments);
  }

  State Apply(const State& state, const std::vector<Object>& arguments) const;

  bool Apply(const std::vector<Object>& arguments, State* state) const {
    return static_cast<bool>(Apply_(arguments, state));
  }

  PartialState Apply(const PartialState& state,
                     const std::vector<Object>& arguments) const;

  /**
   * @return 0 if no change was made, 1 if the value of at least one proposition
   * became explicit, 2 if value of at least one proposition was flipped.
   */
  int Apply(const std::vector<Object>& arguments, PartialState* state) const {
    return ApplyPartial_(arguments, state);
  }

  const VAL::operator_* symbol() const { return symbol_; }

  const Pddl& pddl() const { return *pddl_; }

  const std::string& name() const { return name_; }

  const std::vector<Object>& parameters() const { return parameters_; }

  const ParameterGenerator& parameter_generator() const { return param_gen_; }

  const Formula& preconditions() const { return Preconditions_; }

  const VAL::effect_lists* postconditions() const;

  std::string to_string() const;

  std::string to_string(const std::vector<Object>& arguments) const;

  static std::pair<Action, std::vector<Object>> Parse(
      const Pddl& pddl, const std::string& action_call);

  friend bool operator<(const Action& lhs, const Action& rhs) {
    return lhs.name() < rhs.name();
  }

  friend bool operator==(const Action& lhs, const Action& rhs) {
    return lhs.name() == rhs.name();
  }

  friend std::ostream& operator<<(std::ostream& os, const Action& action);

 protected:
  const VAL::operator_* symbol_ = nullptr;
  const Pddl* pddl_ = nullptr;
  std::string name_;
  std::vector<Object> parameters_;
  ParameterGenerator param_gen_;

  Formula Preconditions_;
  std::function<int(const std::vector<Object>&, State*)> Apply_;
  std::function<int(const std::vector<Object>&, PartialState*)> ApplyPartial_;
};

}  // namespace symbolic

#endif  // SYMBOLIC_ACTION_H_
