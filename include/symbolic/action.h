/**
 * action.h
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_ACTIONS_H_
#define SYMBOLIC_ACTIONS_H_

#include <functional>  // std::function
#include <memory>      // std::shared_ptr
#include <ostream>     // std::ostream
#include <vector>      // std::vector
#include <set>         // std::set
#include <string>      // std::string
#include <utility>     // std::pair

#include "ptree.h"

#include "symbolic/object.h"
#include "symbolic/proposition.h"
#include "symbolic/formula.h"
#include "symbolic/utils/parameter_generator.h"

namespace symbolic {

class Pddl;

class Action {

 public:

  Action(const Pddl& pddl, const VAL::operator_* symbol);

   // action_call can be action(params) or action name
  Action(const Pddl& pddl, const std::string& action_call);

  bool IsValid(const std::set<Proposition>& state,
               const std::vector<Object>& arguments) const {
    return Preconditions_(state, arguments);
  }

  std::set<Proposition> Apply(const std::set<Proposition>& state,
                              const std::vector<Object>& arguments) const;

  void Apply(const std::vector<Object>& arguments, std::set<Proposition>* state) const {
    Apply_(arguments, state);
  }

  const VAL::operator_* symbol() const { return symbol_; }

  const std::string& name() const { return name_; }

  const std::vector<Object>& parameters() const { return parameters_; }

  const ParameterGenerator& parameter_generator() const { return param_gen_; }

  std::string to_string() const;

  std::string to_string(const std::vector<Object>& arguments) const;

 private:

  const VAL::operator_* symbol_;
  std::string name_;
  std::vector<Object> parameters_;
  ParameterGenerator param_gen_;

  Formula Preconditions_;
  std::function<void(const std::vector<Object>&, std::set<Proposition>*)> Apply_;

};

std::pair<Action, std::vector<Object>> ParseAction(const Pddl& pddl, const std::string& action_call);

std::ostream& operator<<(std::ostream& os, const Action& action);

inline bool operator<(const Action& lhs, const Action& rhs) {
  return lhs.name() < rhs.name();
}

inline bool operator==(const Action& lhs, const Action& rhs) {
  return lhs.name() == rhs.name();
}

}  // namespace symbolic

#endif  // SYMBOLIC_ACTIONS_H_
