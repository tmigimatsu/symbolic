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

#include <iostream>       // std::cout, std::ostream
#include <memory>         // std::unique_ptr
#include <set>            // std::set
#include <string>         // std::string
#include <unordered_map>  // std::unordered_map
#include <utility>        // std::pair
#include <vector>         // std::vector

#include "symbolic/action.h"
#include "symbolic/axiom.h"
#include "symbolic/derived_predicate.h"
#include "symbolic/formula.h"
#include "symbolic/object.h"
#include "symbolic/predicate.h"
#include "symbolic/proposition.h"

namespace VAL {

class analysis;

}  // namespace VAL

namespace symbolic_v1 {

/**
 * Main class for manipulating the pddl specification.
 */
class Pddl {
 public:
  using ObjectTypeMap = std::unordered_map<std::string, std::vector<Object>>;

  /**
   * Copy constructor.
   */
  Pddl(const Pddl& pddl) = default;

  /**
   * Move constructor.
   */
  Pddl(Pddl&& pddl) = default;

  /**
   * Copy assignment.
   */
  Pddl& operator=(const Pddl& rhs) = default;

  /**
   * Move assignment.
   */
  Pddl& operator=(Pddl&& rhs) = default;

  /**
   * Destructor.
   */
  virtual ~Pddl();

  /**
   * Parse the pddl specification from the domain and problem files.
   *
   * @param domain_pddl Path to the domain pddl.
   * @param problem_pddl Path to the problem pddl.
   *
   * @seepython{symbolic.Pddl,__init__}
   */
  Pddl(const std::string& domain_pddl, const std::string& problem_pddl);

  /**
   * Evaluate whether the pddl specification is valid using VAL.
   *
   * @param verbose Print diagnostic information.
   * @param os Output stream where diagnostic information should be printed.
   * @returns Whether the pddl specification is valid.
   *
   * @seepython{symbolic.Pddl,is_valid}
   */
  bool IsValid(bool verbose = false, std::ostream& os = std::cout) const;

  /**
   * Apply an action to the given state.
   *
   * The action's preconditions are not checked. The resulting state includes
   * derived predicates.
   *
   * @param state Current state.
   * @param action Action call in the form of `"action(obj_a, obj_b)"`.
   * @returns Next state.
   *
   * @seepython{symbolic.Pddl,next_state}
   */
  State NextState(const State& state, const std::string& action) const;

  /**
   * Apply the derived predicates to the given state.
   */
  State DerivedState(const State& state) const;

  /**
   * Applies the axioms to the given state.
   *
   * @param state Current state.
   * @returns State with axioms applied.
   *
   * @seepython{symbolic.Pddl,consistent_state}
   */
  State ConsistentState(const State& state) const;

  /**
   * Applies the axioms to the given partial state.
   *
   * @param state Current partial state.
   * @returns Partial state with axioms applied.
   *
   * @seepython{symbolic.Pddl,consistent_state}
   */
  PartialState ConsistentState(const PartialState& state) const;

  /**
   * Evaluate whether the action's preconditions are satisfied.
   *
   * @param state Current state.
   * @param action Action call in the form of `"action(obj_a, obj_b)"`.
   * @returns Whether the action can be applied to the state.
   *
   * @seepython{symbolic.Pddl,is_valid_action}
   */
  bool IsValidAction(const State& state, const std::string& action) const;

  /**
   * Evaluates whether the state satisfies the axioms.
   *
   * @param state Current state.
   * @returns Whether the state is valid.
   *
   * @seepython{symbolic.Pddl,is_valid_state}
   */
  bool IsValidState(const State& state) const;

  /**
   * Evaluates whether the partial state satisfies the axioms.
   *
   * Returns false only if a partial state fully satisfies the pre-conditions of
   * the axiom and explicitly does not satisfy the post-conditions. If a
   * proposition in the partial state is unknown, the axiom is assumed to be
   * satisfied.
   *
   * @param state Current partial state.
   * @returns Whether the state is valid.
   *
   * @seepython{symbolic.Pddl,is_valid_state}
   */
  bool IsValidState(const PartialState& state) const;

  /**
   * Evaluate whether the (s, a, s') tuple is valid.
   */
  bool IsValidTuple(const State& state, const std::string& action_call,
                    const State& next_state) const;
  bool IsValidTuple(const std::set<std::string>& state,
                    const std::string& action_call,
                    const std::set<std::string>& next_state) const;

  /**
   * Evaluate whether the goal is satisfied at the given state.
   */
  bool IsGoalSatisfied(const State& state) const { return goal_(state); }
  bool IsGoalSatisfied(const std::set<std::string>& state) const;

  /**
   * Evaluate whether the given action skeleton is valid and satisfies the goal.
   */
  bool IsValidPlan(const std::vector<std::string>& action_skeleton) const;

  /**
   * List the valid arguments for an action from the given state.
   */
  std::vector<std::vector<Object>> ListValidArguments(
      const State& state, const Action& action) const;
  std::vector<std::vector<std::string>> ListValidArguments(
      const std::set<std::string>& state, const std::string& action_name) const;

  /**
   * List the valid actions from the given state.
   */
  std::vector<std::string> ListValidActions(const State& state) const;
  std::vector<std::string> ListValidActions(
      const std::set<std::string>& state) const;

  const VAL::analysis* symbol() const { return analysis_.get(); }

  /**
   * Pddl domain name.
   */
  const std::string& name() const;

  // const VAL::domain& domain() const { return *analysis().the_domain; }

  // const VAL::problem& problem() const { return *analysis().the_problem; }

  const std::string& domain_pddl() const { return domain_pddl_; }

  const std::string& problem_pddl() const { return problem_pddl_; }

  const State& initial_state() const { return initial_state_; }

  const ObjectTypeMap& object_map() const { return object_map_; }

  const std::vector<Object>& objects() const { return objects_; }

  const std::vector<Action>& actions() const { return actions_; }

  const std::vector<Predicate>& predicates() const { return predicates_; }

  const std::vector<Axiom>& axioms() const { return axioms_; }

  const std::vector<DerivedPredicate>& derived_predicates() const {
    return derived_predicates_;
  }

  const StateIndex& state_index() const { return state_index_; }

  const Formula& goal() const { return goal_; }

 private:
  std::unique_ptr<VAL::analysis> analysis_;
  std::string domain_pddl_;
  std::string problem_pddl_;

  std::vector<Object> objects_;
  ObjectTypeMap object_map_;

  std::vector<Action> actions_;
  std::vector<Predicate> predicates_;
  std::vector<Axiom> axioms_;
  std::vector<DerivedPredicate> derived_predicates_;

  StateIndex state_index_;

  State initial_state_;
  Formula goal_;
};

std::set<std::string> Stringify(const State& state);
std::pair<std::set<std::string>, std::set<std::string>> Stringify(
    const PartialState& state);
std::vector<std::string> Stringify(const std::vector<Action>& actions);
std::vector<std::vector<std::string>> Stringify(
    const std::vector<std::vector<Object>>& arguments);
std::vector<std::string> Stringify(const std::vector<Object>& objects);

std::ostream& operator<<(std::ostream& os, const Pddl& pddl);

}  // namespace symbolic_v1

#endif  // SYMBOLIC_PDDL_H_
