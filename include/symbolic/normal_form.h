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

struct FormulaLiterals {
  bool empty() const { return pos.empty() && neg.empty(); }
  size_t size() const { return pos.size() + neg.size(); }

  friend bool operator==(const FormulaLiterals& lhs,
                         const FormulaLiterals& rhs) {
    return lhs.pos == rhs.pos && lhs.neg == rhs.neg;
  }

  friend bool operator<(const FormulaLiterals& lhs,
                        const FormulaLiterals& rhs) {
    return std::tie(lhs.pos, lhs.neg) < std::tie(rhs.pos, rhs.neg);
  }

  friend ostream& operator<<(ostream& os, const FormulaLiterals& lits);

  State pos;
  State neg;
};

struct DisjunctiveFormula {
  using Conjunction = FormulaLiterals;

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
      const std::vector<Object>& arguments) {
    return Create(pddl, formula.symbol(), parameters, arguments);
  }

  static std::optional<DisjunctiveFormula> Create(
      const Pddl& pddl, const VAL::goal* symbol,
      const std::vector<Object>& parameters,
      const std::vector<Object>& arguments);

  static std::optional<DisjunctiveFormula> Create(
      const Pddl& pddl, const VAL::effect_lists* symbol,
      const std::vector<Object>& parameters,
      const std::vector<Object>& arguments);

  static std::optional<DisjunctiveFormula> Create(const Pddl& pddl,
                                                  ConjunctiveFormula&& cnf);

  /**
   * Normalize the pre/post conditions of the given action.
   *
   * If either of the conditions are invalid, this function will return an empty
   * optional.
   *
   * @param pddl Pddl object.
   * @param action_call Action call string.
   * @return Pair of normalized pre/post conditions.
   */
  static std::optional<std::pair<DisjunctiveFormula, DisjunctiveFormula>>
  NormalizeConditions(const Pddl& pddl, const std::string& action_call);

  friend bool operator==(const DisjunctiveFormula& lhs,
                         const DisjunctiveFormula& rhs) {
    return lhs.conjunctions == rhs.conjunctions;
  }

  friend ostream& operator<<(ostream& os, const DisjunctiveFormula& dnf);

  std::vector<Conjunction> conjunctions;
};

struct ConjunctiveFormula {
  using Disjunction = FormulaLiterals;

  ConjunctiveFormula() = default;

  // ConjunctiveFormula(const Pddl& pddl, const Formula& formula,
  //                    const std::vector<Object>& parameters,
  //                    const std::vector<Object>& arguments);

  // ConjunctiveFormula(const DisjunctiveFormula& dnf);

  friend ostream& operator<<(ostream& os, const ConjunctiveFormula& cnf);

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
