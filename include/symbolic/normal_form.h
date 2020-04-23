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
#include <vector>      // std::vector
#include <utility>     // std::pair

#include "symbolic/action.h"
#include "symbolic/formula.h"

namespace symbolic {

struct ConjunctiveFormula;

struct FormulaLiterals {

  bool empty() const { return pos.empty() && neg.empty(); }
  size_t size() const { return pos.size() + neg.size(); }

  std::vector<Proposition> pos;
  std::vector<Proposition> neg;

};

struct DisjunctiveFormula {

  using Conjunction = FormulaLiterals;

  DisjunctiveFormula() {}

  DisjunctiveFormula(std::vector<Conjunction>&& conjunctions)
      : conjunctions(std::move(conjunctions)) {}

  DisjunctiveFormula(const Pddl& pddl, const Formula& formula,
                     const std::vector<Object>& parameters,
                     const std::vector<Object>& arguments)
      : DisjunctiveFormula(pddl, formula.symbol(), parameters, arguments) {}

  DisjunctiveFormula(const Pddl& pddl, const VAL::goal* symbol,
                     const std::vector<Object>& parameters,
                     const std::vector<Object>& arguments);

  DisjunctiveFormula(const Pddl& pddl, const VAL::effect_lists* symbol,
                     const std::vector<Object>& parameters,
                     const std::vector<Object>& arguments);

  DisjunctiveFormula(ConjunctiveFormula&& cnf);

  bool empty() const { return conjunctions.empty(); }

  std::vector<Conjunction> conjunctions;

};

struct ConjunctiveFormula {

  using Disjunction = FormulaLiterals;

  ConjunctiveFormula() {}

  // ConjunctiveFormula(const Pddl& pddl, const Formula& formula,
  //                    const std::vector<Object>& parameters,
  //                    const std::vector<Object>& arguments);

  // ConjunctiveFormula(const DisjunctiveFormula& dnf);

  std::vector<Disjunction> disjunctions;

};

inline std::pair<DisjunctiveFormula, DisjunctiveFormula>
NormalizeConditions(const Pddl& pddl, const std::string& action_call) {
  const auto aa = ParseAction(pddl, action_call);
  return { DisjunctiveFormula(pddl, aa.first.preconditions().symbol(), aa.first.parameters(), aa.second),
           DisjunctiveFormula(pddl, aa.first.postconditions(), aa.first.parameters(), aa.second) };
}

// bool Simplify(DisjunctiveFormula* dnf);

// DisjunctiveFormula Disjoin(std::vector<DisjunctiveFormula>&& dnfs);

// DisjunctiveFormula Conjoin(const std::vector<DisjunctiveFormula>& dnfs);

DisjunctiveFormula Negate(DisjunctiveFormula&& dnf);

// ConjunctiveFormula Flip(DisjunctiveFormula&& cnf);

inline bool operator==(const FormulaLiterals& lhs, const FormulaLiterals& rhs) {
  return lhs.pos == rhs.pos && lhs.neg == rhs.neg;
}
inline bool operator<(const FormulaLiterals& lhs, const FormulaLiterals& rhs) {
  return std::tie(lhs.pos, lhs.neg) < std::tie(rhs.pos, rhs.neg);
}

inline bool operator==(const DisjunctiveFormula& lhs, const DisjunctiveFormula& rhs) {
  return lhs.conjunctions == rhs.conjunctions;
}

ostream& operator<<(ostream& os, const DisjunctiveFormula::Conjunction& conj);
ostream& operator<<(ostream& os, const DisjunctiveFormula& dnf);

}  // namespace symbolic

#endif  // SYMBOLIC_NORMAL_FORM_H_
