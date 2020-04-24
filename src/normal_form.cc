/**
 * normal_form.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: April 17, 2020
 * Authors: Toki Migimatsu
 */

#include "symbolic/normal_form.h"

#include <algorithm>  // std::swap, std::all_of
#include <cassert>    // assert
#include <iterator>   // std::make_move_iterator
#include <optional>   // std::optional

#include "symbolic/pddl.h"
#include "symbolic/utils/combination_generator.h"

namespace {

using ::symbolic::DisjunctiveFormula;
using ::symbolic::Proposition;

std::optional<bool> EvaluateEquals(const Proposition& prop) {
  if (prop.name() != "=") return {};
  assert(prop.arguments().size() == 2);
  return prop.arguments()[0] == prop.arguments()[1];
}

std::optional<bool> Negate(const std::optional<bool>& x) {
  return x.has_value() ? !*x : std::optional<bool>{};
}

template <typename T>
bool Contains(const std::vector<T> vals, const T& val) {
  assert(std::is_sorted(vals.begin(), vals.end()));
  const auto it = std::lower_bound(vals.begin(), vals.end(), val);
  return it != vals.end() && *it == val;
}
template <typename T>
bool Contains(const std::set<T> vals, const T& val) {
  return vals.find(val) != vals.end();
}

template <typename T>
void SortUnique(std::vector<T>* vals) {
  std::sort(vals->begin(), vals->end());
  auto last = std::unique(vals->begin(), vals->end());
  vals->erase(last, vals->end());
}

bool IsSubset(const DisjunctiveFormula::Conjunction& sub,
              const DisjunctiveFormula::Conjunction& super) {
  if (sub.size() > super.size()) return false;

  for (const Proposition& prop : sub.pos) {
    if (!super.pos.contains(prop)) return false;
  }
  for (const Proposition& prop : sub.neg) {
    if (!super.neg.contains(prop)) return false;
  }
  return true;
}

// Try to insert conj as a subset of one of the elements in the conjunctions.
// If conj is a superset, keep the subset. If conj is neither a subset nor
// superset, return false.
bool TryInsertSubset(
    const DisjunctiveFormula::Conjunction& conj,
    std::vector<DisjunctiveFormula::Conjunction>* conjunctions) {
  for (const DisjunctiveFormula::Conjunction& conj_i : *conjunctions) {
    // If current conjunction is a superset, don't add it
    if (IsSubset(conj_i, conj)) return true;
  }

  bool is_subset = false;
  for (DisjunctiveFormula::Conjunction& conj_i : *conjunctions) {
    // Current conjunction is a subset: replace previous one
    if (IsSubset(conj, conj_i)) {
      is_subset = true;
      conj_i = conj;
    }
  }

  if (is_subset) {
    SortUnique(conjunctions);
  }

  return is_subset;
}

}  // namespace

namespace symbolic {

std::optional<bool> Evaluate(const Pddl& pddl,
                             const DisjunctiveFormula::Conjunction& conj) {
  // Evaluate =
  if (conj.size() == 1) {
    return conj.neg.empty() ? EvaluateEquals(*conj.pos.begin())
                            : ::Negate(EvaluateEquals(*conj.neg.begin()));
  }

  // Ensure pos and neg sets don't overlap
  for (const Proposition& prop : conj.pos) {
    if (conj.neg.contains(prop)) return false;
  }

  // Ensure axioms are satisfied
  for (const Axiom& axiom : pddl.axioms()) {
    if (!axiom.IsConsistent(conj.pos)) return false;
  }
  return {};
}

std::optional<DisjunctiveFormula> Simplify(const Pddl& pddl,
                                           DisjunctiveFormula&& dnf) {
  DisjunctiveFormula ret;
  ret.conjunctions.reserve(dnf.conjunctions.size());
  for (DisjunctiveFormula::Conjunction& conj : dnf.conjunctions) {
    const std::optional<bool> is_true = Evaluate(pddl, conj);
    if (!is_true.has_value()) {
      if (!TryInsertSubset(conj, &ret.conjunctions)) {
        // Current conjunction is not a subset or superset: append it
        ret.conjunctions.push_back(std::move(conj));
      }
    } else if (*is_true) {
      // Conjunction is true: short-circuit disjunction and return empty formula
      ret.conjunctions.clear();
      return std::move(ret);
    }
    // Conjunction is false: discard it from the disjunction
  }

  // If all conjunctions were false, return null
  if (ret.conjunctions.empty()) return {};

  // Sort conjunctions
  SortUnique(&ret.conjunctions);

  return ret;
}

std::optional<DisjunctiveFormula> Disjoin(
    const Pddl& pddl, std::vector<DisjunctiveFormula>&& dnfs) {
  DisjunctiveFormula disj;
  for (DisjunctiveFormula& dnf : dnfs) {
    disj.conjunctions.insert(disj.conjunctions.end(),
                             std::make_move_iterator(dnf.conjunctions.begin()),
                             std::make_move_iterator(dnf.conjunctions.end()));
  }
  return Simplify(pddl, std::move(disj));
}

std::optional<DisjunctiveFormula> Conjoin(
    const Pddl& pddl, const std::vector<DisjunctiveFormula>& dnfs) {
  // ((a | b) & (c | d) & (e | f))
  // ((a & c & e) | ...)
  DisjunctiveFormula conj;

  // Create combination generator for conjunctions
  std::vector<const std::vector<DisjunctiveFormula::Conjunction>*> conjunctions;
  conjunctions.reserve(dnfs.size());
  for (const DisjunctiveFormula& dnf : dnfs) {
    if (dnf.empty()) continue;
    conjunctions.push_back(&dnf.conjunctions);
  }
  CombinationGenerator<const std::vector<DisjunctiveFormula::Conjunction>> gen(
      conjunctions);

  // Iterate over all combinations
  for (const std::vector<DisjunctiveFormula::Conjunction>& combo : gen) {
    DisjunctiveFormula::Conjunction term;
    for (const DisjunctiveFormula::Conjunction& term_i : combo) {
      term.pos.insert(term_i.pos.begin(), term_i.pos.end());
      term.neg.insert(term_i.neg.begin(), term_i.neg.end());
    }
    conj.conjunctions.push_back(std::move(term));
  }

  return Simplify(pddl, std::move(conj));
}

ConjunctiveFormula Flip(DisjunctiveFormula&& dnf) {
  ConjunctiveFormula cnf;
  cnf.disjunctions = std::move(dnf.conjunctions);
  return cnf;
}

std::vector<DisjunctiveFormula> Convert(ConjunctiveFormula&& cnf) {
  // ((a | b) & (c | d) & (e | f))
  // [(a | b), (c | d), (e | f))
  std::vector<DisjunctiveFormula> dnfs;
  dnfs.reserve(cnf.disjunctions.size());
  for (ConjunctiveFormula::Disjunction& disj : cnf.disjunctions) {
    DisjunctiveFormula dnf;
    dnf.conjunctions.reserve(disj.size());
    for (Proposition& prop : disj.pos) {
      dnf.conjunctions.push_back({{std::move(prop)}, {}});
    }
    for (Proposition& prop : disj.neg) {
      dnf.conjunctions.push_back({{}, {std::move(prop)}});
    }
    dnfs.push_back(std::move(dnf));
  }
  return dnfs;
}

DisjunctiveFormula::DisjunctiveFormula(const Pddl& pddl,
                                       ConjunctiveFormula&& cnf)
    : conjunctions(
          Conjoin(pddl, Convert(std::move(cnf))).value().conjunctions) {}

DisjunctiveFormula Negate(const Pddl& pddl, DisjunctiveFormula&& dnf) {
  // !((a & b) | (c & d) | (e & f))

  // ((!a & !b) | (!c & !d) | (!e & !f))
  for (DisjunctiveFormula::Conjunction& conj : dnf.conjunctions) {
    std::swap(conj.pos, conj.neg);
  }

  // ((!a | !b) & (!c | !d) & (!e | !f))
  ConjunctiveFormula cnf = Flip(std::move(dnf));

  // ((!a & !c & !e) | ...)
  return DisjunctiveFormula(pddl, std::move(cnf));
}

DisjunctiveFormula::DisjunctiveFormula(const Pddl& pddl,
                                       const VAL::goal* symbol,
                                       const std::vector<Object>& parameters,
                                       const std::vector<Object>& arguments) {
  // Proposition
  const auto* simple_goal = dynamic_cast<const VAL::simple_goal*>(symbol);
  if (simple_goal != nullptr) {
    const VAL::proposition* prop = simple_goal->getProp();
    const std::string name_predicate = prop->head->getName();
    const std::vector<Object> prop_params =
        symbolic::ConvertObjects(pddl, prop->args);
    const auto Apply = CreateApplicationFunction(parameters, prop_params);

    conjunctions = {{{Proposition(name_predicate, Apply(arguments))}, {}}};
    return;
  }

  // Conjunction
  const auto* conj_goal = dynamic_cast<const VAL::conj_goal*>(symbol);
  if (conj_goal != nullptr) {
    const VAL::goal_list* goals = conj_goal->getGoals();
    std::vector<DisjunctiveFormula> conj_terms;
    conj_terms.reserve(goals->size());
    for (const VAL::goal* goal : *goals) {
      conj_terms.emplace_back(pddl, goal, parameters, arguments);
    }
    try {
      conjunctions = Conjoin(pddl, conj_terms)->conjunctions;
    } catch (...) {
      throw "DisjunctiveFormula(): Invalid conjunction.";
    }
    return;
  }

  // Disjunction
  const auto* disj_goal = dynamic_cast<const VAL::disj_goal*>(symbol);
  if (disj_goal != nullptr) {
    const VAL::goal_list* goals = disj_goal->getGoals();
    std::vector<DisjunctiveFormula> disj_terms;
    disj_terms.reserve(goals->size());
    for (const VAL::goal* goal : *goals) {
      disj_terms.emplace_back(pddl, goal, parameters, arguments);
    }
    try {
      conjunctions = Disjoin(pddl, std::move(disj_terms))->conjunctions;
    } catch (...) {
      throw "DisjunctiveFormula(): Invalid disjunction.";
    }
    return;
  }

  // Negation
  const auto* neg_goal = dynamic_cast<const VAL::neg_goal*>(symbol);
  if (neg_goal != nullptr) {
    const VAL::goal* goal = neg_goal->getGoal();
    conjunctions =
        Negate(pddl, DisjunctiveFormula(pddl, goal, parameters, arguments))
            .conjunctions;
    return;
  }

  // Forall and exists
  const auto* qfied_goal = dynamic_cast<const VAL::qfied_goal*>(symbol);
  if (qfied_goal != nullptr) {
    const VAL::goal* goal = qfied_goal->getGoal();

    // Create qfied parameters
    std::vector<Object> qfied_params = parameters;
    std::vector<Object> types =
        symbolic::ConvertObjects(pddl, qfied_goal->getVars());
    qfied_params.insert(qfied_params.end(), types.begin(), types.end());

    // Loop over qfied arguments
    std::vector<DisjunctiveFormula> qfied_terms;
    symbolic::ParameterGenerator gen(pddl.object_map(), types);
    for (const std::vector<Object>& qfied_objs : gen) {
      // Create qfied arguments
      std::vector<Object> qfied_args = arguments;
      qfied_args.insert(qfied_args.end(), qfied_objs.begin(), qfied_objs.end());

      qfied_terms.emplace_back(pddl, goal, qfied_params, qfied_args);
    }

    switch (qfied_goal->getQuantifier()) {
      case VAL::quantifier::E_FORALL:
        try {
          conjunctions = Conjoin(pddl, qfied_terms)->conjunctions;
        } catch (...) {
          throw "DisjunctiveFormula(): Invalid forall.";
        }
        return;
      case VAL::quantifier::E_EXISTS:
        try {
          conjunctions = Disjoin(pddl, std::move(qfied_terms))->conjunctions;
        } catch (...) {
          throw "DisjunctiveFormula(): Invalid exists.";
        }
        return;
    }
  }
}

DisjunctiveFormula::DisjunctiveFormula(const Pddl& pddl,
                                       const VAL::effect_lists* symbol,
                                       const std::vector<Object>& parameters,
                                       const std::vector<Object>& arguments) {
  std::vector<DisjunctiveFormula> dnfs;

  // Forall effects
  for (const VAL::forall_effect* effect : symbol->forall_effects) {
    std::vector<Object> forall_params = parameters;
    const std::vector<Object> types =
        symbolic::ConvertObjects(pddl, effect->getVarsList());
    forall_params.insert(forall_params.end(), types.begin(), types.end());

    // Loop over forall arguments
    symbolic::ParameterGenerator gen(pddl.object_map(), types);
    for (const std::vector<Object>& forall_objs : gen) {
      // Create qfied arguments
      std::vector<Object> forall_args = arguments;
      forall_args.insert(forall_args.end(), forall_objs.begin(),
                         forall_objs.end());

      dnfs.emplace_back(pddl, effect->getEffects(), forall_params, forall_args);
    }
  }

  // Add effects
  DisjunctiveFormula::Conjunction simple;
  simple.pos.reserve(symbol->add_effects.size());
  for (const VAL::simple_effect* effect : symbol->add_effects) {
    const std::string name_predicate = effect->prop->head->getName();
    const std::vector<Object> effect_params =
        symbolic::ConvertObjects(pddl, effect->prop->args);
    const auto Apply = CreateApplicationFunction(parameters, effect_params);

    simple.pos.emplace(name_predicate, Apply(arguments));
  }

  // Del effects
  simple.neg.reserve(symbol->del_effects.size());
  for (const VAL::simple_effect* effect : symbol->del_effects) {
    const std::string name_predicate = effect->prop->head->getName();
    const std::vector<Object> effect_params =
        symbolic::ConvertObjects(pddl, effect->prop->args);
    const auto Apply = CreateApplicationFunction(parameters, effect_params);

    simple.neg.emplace(name_predicate, Apply(arguments));
  }

  if (!simple.empty()) {
    dnfs.push_back({{std::move(simple)}});
  }

  // Cond effects
  for (const VAL::cond_effect* effect : symbol->cond_effects) {
    DisjunctiveFormula condition;
    try {
      condition = DisjunctiveFormula(pddl, effect->getCondition(), parameters,
                                     arguments);
    } catch (...) {
      // Condition always false
      continue;
    }

    DisjunctiveFormula result(pddl, effect->getEffects(), parameters,
                              arguments);

    if (condition.empty()) {
      // Condition always true
      dnfs.push_back(std::move(result));
      continue;
    }

    try {
      dnfs.push_back(*Disjoin(
          pddl, {Negate(pddl, std::move(condition)), std::move(result)}));
    } catch (...) {
      std::cerr << condition << std::endl;
      std::cerr << result << std::endl;
      throw "DisjunctiveFormula(): Invalid condition.";
    }
  }

  try {
    conjunctions = Conjoin(pddl, dnfs)->conjunctions;
  } catch (...) {
    throw "DisjunctiveFormula(): Invalid effects.";
  }
}

std::optional<DisjunctiveFormula> DisjunctiveFormula::Create(
    const Pddl& pddl, const Formula& formula,
    const std::vector<Object>& parameters,
    const std::vector<Object>& arguments) {
  try {
    return Simplify(pddl,
                    DisjunctiveFormula(pddl, formula, parameters, arguments));
  } catch (...) {
    return {};
  }
}

ostream& operator<<(ostream& os, const DisjunctiveFormula::Conjunction& conj) {
  os << "(and" << std::endl;
  for (const Proposition& prop : conj.pos) {
    os << "\t" << prop << std::endl;
  }
  for (const Proposition& prop : conj.neg) {
    os << "\tnot " << prop << std::endl;
  }
  os << ")" << std::endl;
  return os;
}
ostream& operator<<(ostream& os, const DisjunctiveFormula& dnf) {
  os << "(or" << std::endl;
  for (const DisjunctiveFormula::Conjunction& conj : dnf.conjunctions) {
    os << "\t(and" << std::endl;
    for (const Proposition& prop : conj.pos) {
      os << "\t\t" << prop << std::endl;
    }
    for (const Proposition& prop : conj.neg) {
      os << "\t\tnot " << prop << std::endl;
    }
    os << "\t)" << std::endl;
  }
  os << ")" << std::endl;
  return os;
}

}  // namespace symbolic
