/**
 * normal_form.h
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: April 17, 2018
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_NORMAL_FORM_H_
#define SYMBOLIC_NORMAL_FORM_H_

#include <functional>  // std::function
#include <ostream>     // std::ostream
#include <set>         // std::set
#include <utility>     // std::pair
#include <vector>      // std::vector

#include "symbolic/action.h"
#include "symbolic/formula.h"
#include "symbolic/state.h"

namespace symbolic {

struct ConjunctiveFormula;

struct DisjunctiveFormula {
  using Conjunction = PartialState;

  DisjunctiveFormula() = default;

  explicit DisjunctiveFormula(std::vector<Conjunction>&& conjunctions)
      : conjunctions(std::move(conjunctions)) {}

  DisjunctiveFormula(std::initializer_list<Conjunction> l) : conjunctions(l) {}

  DisjunctiveFormula(const Pddl& pddl, const Formula& formula,
                     const std::vector<Object>& parameters,
                     const std::vector<Object>& arguments)
      : DisjunctiveFormula(pddl, formula.symbol(), parameters, arguments) {}

  DisjunctiveFormula(const Pddl& pddl, const VAL::goal* symbol,
                     const std::vector<Object>& parameters,
                     const std::vector<Object>& arguments)
      : DisjunctiveFormula(
            Create(pddl, symbol, parameters, arguments).value()) {}

  DisjunctiveFormula(const Pddl& pddl, const VAL::effect_lists* symbol,
                     const std::vector<Object>& parameters,
                     const std::vector<Object>& arguments)
      : DisjunctiveFormula(
            Create(pddl, symbol, parameters, arguments).value()) {}

  DisjunctiveFormula(const Pddl& pddl, ConjunctiveFormula&& cnf)
      : DisjunctiveFormula(Create(pddl, std::move(cnf)).value()) {}

  bool empty() const { return conjunctions.empty(); }

  static std::optional<DisjunctiveFormula> Create(
      const Pddl& pddl, const Formula& formula,
      const std::vector<Object>& parameters,
      const std::vector<Object>& arguments, bool apply_axioms = false) {
    return Create(pddl, formula.symbol(), parameters, arguments, apply_axioms);
  }

  static std::optional<DisjunctiveFormula> Create(
      const Pddl& pddl, const VAL::goal* symbol,
      const std::vector<Object>& parameters,
      const std::vector<Object>& arguments, bool apply_axioms = false);

  static std::optional<DisjunctiveFormula> Create(
      const Pddl& pddl, const VAL::effect_lists* symbol,
      const std::vector<Object>& parameters,
      const std::vector<Object>& arguments, bool apply_axioms = false);

  static std::optional<DisjunctiveFormula> Create(const Pddl& pddl,
                                                  ConjunctiveFormula&& cnf,
                                                  bool apply_axioms = false);

  /**
   * Normalize the pre/post conditions of the given action.
   *
   * If either of the conditions are invalid, this function will return an empty
   * optional.
   *
   * @param pddl Pddl object.
   * @param action_call Action call string.
   * @param apply_axioms Whether to apply the axioms to the pre/post conditions.
   * @return Pair of normalized pre/post conditions.
   *
   * @seepython{symbolic.DisjunctiveFormula,normalize_conditions}
   */
  static std::optional<std::pair<DisjunctiveFormula, DisjunctiveFormula>>
  NormalizeConditions(const Pddl& pddl, const std::string& action_call,
                      bool apply_axioms = false);

  static std::optional<DisjunctiveFormula> NormalizePreconditions(
      const Pddl& pddl, const std::string& action_call,
      bool apply_axioms = false);

  static std::optional<DisjunctiveFormula> NormalizePostconditions(
      const Pddl& pddl, const std::string& action_call,
      bool apply_axioms = false);

  static std::optional<DisjunctiveFormula> NormalizeGoal(
      const Pddl& pddl, bool apply_axioms = false);

  friend bool operator==(const DisjunctiveFormula& lhs,
                         const DisjunctiveFormula& rhs) {
    return lhs.conjunctions == rhs.conjunctions;
  }

  friend std::ostream& operator<<(std::ostream& os,
                                  const DisjunctiveFormula& dnf);

  std::vector<Conjunction> conjunctions;
};

struct ConjunctiveFormula {
  using Disjunction = PartialState;

  ConjunctiveFormula() = default;

  // ConjunctiveFormula(const Pddl& pddl, const Formula& formula,
  //                    const std::vector<Object>& parameters,
  //                    const std::vector<Object>& arguments);

  // ConjunctiveFormula(const DisjunctiveFormula& dnf);

  friend std::ostream& operator<<(std::ostream& os,
                                  const ConjunctiveFormula& cnf);

  std::vector<Disjunction> disjunctions;
};

// bool Simplify(DisjunctiveFormula* dnf);

// DisjunctiveFormula Disjoin(std::vector<DisjunctiveFormula>&& dnfs);

// DisjunctiveFormula Conjoin(const std::vector<DisjunctiveFormula>& dnfs);

std::optional<DisjunctiveFormula> Negate(const Pddl& pddl,
                                         DisjunctiveFormula&& dnf);

// ConjunctiveFormula Flip(DisjunctiveFormula&& cnf);

}  // namespace symbolic

#endif  // SYMBOLIC_NORMAL_FORM_H_
