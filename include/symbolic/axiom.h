/**
 * axiom.h
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: April 23, 2020
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_AXIOM_H_
#define SYMBOLIC_AXIOM_H_

#include "symbolic/action.h"
#include "symbolic/normal_form.h"

namespace symbolic {

class Axiom : public Action {
 public:
  Axiom(const Pddl& pddl, const VAL::operator_* symbol);

  /**
   * Determine whether the axiom is satisfied in the given state.
   *
   * @param state State to evaluate.
   * @returns Whether the state is consistent with this axiom.
   */
  bool IsConsistent(const State& state) const;

  /**
   * Determine whether the axiom is satisfied in the given partial state.
   *
   * @param state Partial state to evaluate.
   * @returns Whether the state is consistent with this axiom.
   */
  bool IsConsistent(const PartialState& state) const;

  /**
   * Determine whether all axioms are satisfied in the given partial state.
   *
   * Returns false only if a partial state fully satisfies the pre-conditions of
   * the axiom and explicitly does not satisfy the post-conditions. If a
   * proposition in the partial state is unknown, the axiom is assumed to be
   * satisfied.
   *
   * @param state Partial state to evaluate.
   * @returns Whether the state is consistent with this axiom.
   */
  static bool IsConsistent(const std::vector<std::shared_ptr<Axiom>>& axioms,
                           const PartialState& state);

  /**
   * Iterate over all argument combinations and apply the implication whenever
   * the context is valid.
   */
  State Apply(const State& state) const;
  bool Apply(State* state) const;

  /**
   * Iterate over all argument combinations and apply the implication whenever
   * the context is valid.
   */
  PartialState Apply(const PartialState& state) const;
  int Apply(PartialState* state) const;

  /**
   * Predicate used in the axiom context.
   */
  const SignedProposition& context() const { return context_; }

  friend std::ostream& operator<<(std::ostream& os, const Axiom& axiom);

  /**
   * Creates a function that takes action_args and returns axiom_args based on
   * positional indices of the axiom context proposition. If the action_args are
   * not consistent with the axiom context, returns an empty vector.
   */
  static std::optional<
      std::function<const std::vector<Object>*(const std::vector<Object>&)>>
  CreateApplicationFunction(const std::vector<Object>& action_params,
                            const std::vector<Object>& action_prop_params,
                            const std::vector<Object>& axiom_params,
                            const std::vector<Object>& axiom_prop_params);

 private:
  bool IsConsistent(PartialState* state, bool* is_changed) const;

  std::vector<std::vector<Object>> arguments_;
  SignedProposition context_;
  std::string formula_;
};

}  // namespace symbolic

#endif  // SYMBOLIC_AXIOM_H_
