/**
 * pddl.h
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_PDDL_H_
#define SYMBOLIC_PDDL_H_

#include <iostream>  // std::cout, std::ostream
#include <memory>    // std::unique_ptr
#include <set>       // std::set
#include <string>    // std::string
#include <vector>    // std::vector

#include "ptree.h"

#include "symbolic/action.h"
#include "symbolic/formula.h"
#include "symbolic/object.h"
#include "symbolic/proposition.h"

namespace symbolic {

class Pddl {

 public:

  using ObjectTypeMap = std::map<std::string, std::vector<Object>>;

  Pddl(const std::string& domain_pddl, const std::string& problem_pddl);

  bool IsValid(bool verbose = false, std::ostream& output = std::cout) const;

  std::set<Proposition> NextState(const std::set<Proposition>& state,
                                  const std::string& action_call) const;
  std::set<std::string> NextState(const std::set<std::string>& state,
                                  const std::string& action_call) const;

  bool IsValidAction(const std::set<Proposition>& state,
                     const std::string& action_call) const;
  bool IsValidAction(const std::set<std::string>& state,
                     const std::string& action_call) const;

  bool IsValidTuple(const std::set<Proposition>& state,
                    const std::string& action_call,
                    const std::set<Proposition>& next_state) const;
  bool IsValidTuple(const std::set<std::string>& state,
                    const std::string& action_call,
                    const std::set<std::string>& next_state) const;

  bool IsGoalSatisfied(const std::set<Proposition>& state) const { return goal_(state); }
  bool IsGoalSatisfied(const std::set<std::string>& state) const;

  bool IsValidPlan(const std::vector<std::string>& action_skeleton) const;

  std::vector<std::vector<Object>> ListValidArguments(const std::set<Proposition>& state,
                                                      const Action& action) const;
  std::vector<std::vector<std::string>> ListValidArguments(const std::set<std::string>& state,
                                                           const std::string& action_name) const;

  std::vector<std::string> ListValidActions(const std::set<Proposition>& state) const;
  std::vector<std::string> ListValidActions(const std::set<std::string>& state) const;

  const VAL::analysis& analysis() const { return *analysis_; }

  const VAL::domain& domain() const { return domain_; }

  const VAL::problem& problem() const { return problem_; }

  const std::set<Proposition>& initial_state() const { return initial_state_; }
  const std::set<std::string>& initial_state_str() const { return initial_state_str_; }

  const ObjectTypeMap& object_map() const { return object_map_; }

  const std::vector<Object>& objects() const { return objects_; }

  const std::set<Action>& actions() const { return actions_; }

  const Formula& goal() const { return goal_; }

 private:

  const std::unique_ptr<VAL::analysis> analysis_;
  const VAL::domain& domain_;
  const VAL::problem& problem_;

  const std::vector<Object> objects_;
  const ObjectTypeMap object_map_;

  const std::set<Action> actions_;

  const std::set<Proposition> initial_state_;
  const std::set<std::string> initial_state_str_;
  const Formula goal_;

};

std::ostream& operator<<(std::ostream& os, const Pddl& pddl);

}  // namespace symbolic

namespace VAL {

std::ostream& operator<<(std::ostream& os, const VAL::domain& domain);

std::ostream& operator<<(std::ostream& os, const VAL::problem& problem);

std::ostream& operator<<(std::ostream& os, const VAL::simple_effect& effect);

std::ostream& operator<<(std::ostream& os, const VAL::var_symbol_list& args);

std::ostream& operator<<(std::ostream& os, const VAL::parameter_symbol_list& args);

}  // namespace VAL

#endif  // SYMBOLIC_PDDL_H_
