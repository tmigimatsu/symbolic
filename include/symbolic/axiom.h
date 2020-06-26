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

namespace symbolic {

class Axiom : public Action {
 public:
  Axiom(const Pddl& pddl, const VAL::operator_* symbol);

  /**
   * Determine whether the axiom is satisfied.
   */
  bool IsConsistent(const State& state) const;

  /**
   * Iterate over all argument combinations and apply the implication whenever
   * the context is valid.
   */
  State Apply(const State& state) const;
  bool Apply(State* state) const;

  friend std::ostream& operator<<(std::ostream& os, const Axiom& axiom);

 private:
  std::vector<std::vector<Object>> arguments_;
  std::string formula_;
};

}  // namespace symbolic

#endif  // SYMBOLIC_AXIOM_H_
