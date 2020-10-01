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
   * Determine whether the axiom is satisfied.
   */
  bool IsConsistent(const State& state) const;
  bool IsConsistent(const PartialState& state) const;

  static bool IsConsistent(const std::vector<Axiom>& axioms,
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

  friend std::ostream& operator<<(std::ostream& os, const Axiom& axiom);

 private:
  bool IsConsistent(PartialState* state, bool* is_changed) const;

  std::vector<std::vector<Object>> arguments_;
  std::string formula_;
};

}  // namespace symbolic

#endif  // SYMBOLIC_AXIOM_H_
