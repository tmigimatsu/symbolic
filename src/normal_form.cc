/**
 * normal_form.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: April 17, 2020
 * Authors: Toki Migimatsu
 */

#include "symbolic/normal_form.h"

#include <VAL/ptree.h>

#include <algorithm>  // std::swap, std::all_of
#include <cassert>    // assert
#include <iterator>   // std::make_move_iterator
#include <optional>   // std::optional

#include "symbolic/pddl.h"
#include "symbolic/utils/combination_generator.h"

namespace {

using ::symbolic_v1::DisjunctiveFormula;
using ::symbolic_v1::Object;
using ::symbolic_v1::ParameterGenerator;
using ::symbolic_v1::PartialState;
using ::symbolic_v1::Pddl;
using ::symbolic_v1::Proposition;

std::optional<bool> Negate(const std::optional<bool>& x) {
  return x.has_value() ? !*x : std::optional<bool>{};
}

std::optional<bool> EvaluateEquals(const Proposition& prop) {
  if (prop.name() != "=") return {};
  assert(prop.arguments().size() == 2);
  const Object& a = prop.arguments()[0];
  const Object& b = prop.arguments()[1];
  if (dynamic_cast<const VAL::var_symbol*>(a.symbol()) !=
      dynamic_cast<const VAL::var_symbol*>(b.symbol())) {
    // Cannot evaluate if one is a parameter (var_symbol) and the other is an
    // argument (parameter_symbol)
    return {};
  }
  return a == b;
}

std::optional<bool> EvaluateEquals(const PartialState& formula) {
  assert(formula.size() == 1);
  return formula.neg().empty() ? EvaluateEquals(*formula.pos().begin())
                               : Negate(EvaluateEquals(*formula.neg().begin()));
}

std::optional<bool> EvaluateType(const Pddl& pddl, const Proposition& prop) {
  if (pddl.object_map().find(prop.name()) == pddl.object_map().end()) return {};
  assert(prop.arguments().size() == 1);
  return prop.arguments()[0].type().IsSubtype(prop.name());
}

std::optional<bool> EvaluateType(const Pddl& pddl,
                                 const PartialState& formula) {
  assert(formula.size() == 1);
  return formula.neg().empty()
             ? EvaluateType(pddl, *formula.pos().begin())
             : Negate(EvaluateType(pddl, *formula.neg().begin()));
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

  for (const Proposition& prop : sub.pos()) {
    if (!super.pos().contains(prop)) return false;
  }
  for (const Proposition& prop : sub.neg()) {
    if (!super.neg().contains(prop)) return false;
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

namespace symbolic_v1 {

std::optional<bool> Evaluate(const Pddl& pddl,
                             const DisjunctiveFormula::Conjunction& conj) {
  // Evaluate =, type
  if (conj.size() == 1) {
    std::optional<bool> eval = EvaluateEquals(conj);
    if (eval.has_value()) return eval;
    eval = EvaluateType(pddl, conj);
    if (eval.has_value()) return eval;
  }

  // Ensure pos and neg sets don't overlap
  if (!conj.IsConsistent()) return false;

  // Ensure axioms are satisfied
  if (!Axiom::IsConsistent(pddl.axioms(), conj)) return false;
  return {};
}

std::optional<DisjunctiveFormula> Simplify(const Pddl& pddl,
                                           DisjunctiveFormula&& dnf) {
  if (dnf.empty()) return std::move(dnf);

  DisjunctiveFormula ret;
  ret.conjunctions.reserve(dnf.conjunctions.size());
  for (DisjunctiveFormula::Conjunction& conj : dnf.conjunctions) {
    // Apply derived predicates to conjunction
    // TODO(tmigimatsu): Need to generate all combinations of unspecified
    // predicates (ones not in conj.pos nor conj.neg) to satisfy all possible
    // derived predicate conditions.
    // DerivedPredicate::Apply(pddl.derived_predicates(), &conj.pos);

    // Evaluate conjunction
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
  if (ret.empty()) return {};

  // Sort conjunctions
  SortUnique(&ret.conjunctions);

  return std::move(ret);
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
    State pos;
    State neg;
    for (const DisjunctiveFormula::Conjunction& term_i : combo) {
      pos.insert(term_i.pos().begin(), term_i.pos().end());
      neg.insert(term_i.neg().begin(), term_i.neg().end());
    }
    conj.conjunctions.emplace_back(std::move(pos), std::move(neg));
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
    for (Proposition& prop : disj.pos()) {
      dnf.conjunctions.push_back(PartialState({std::move(prop)}, {}));
    }
    for (Proposition& prop : disj.neg()) {
      dnf.conjunctions.push_back(PartialState({}, {std::move(prop)}));
    }
    dnfs.push_back(std::move(dnf));
  }
  return dnfs;
}

std::optional<DisjunctiveFormula> DisjunctiveFormula::Create(
    const Pddl& pddl, ConjunctiveFormula&& cnf) {
  return Conjoin(pddl, Convert(std::move(cnf)));
}

std::optional<DisjunctiveFormula> Negate(const Pddl& pddl,
                                         DisjunctiveFormula&& dnf) {
  // !((a & b) | (c & d) | (e & f))

  // ((!a & !b) | (!c & !d) | (!e & !f))
  for (DisjunctiveFormula::Conjunction& conj : dnf.conjunctions) {
    std::swap(conj.pos(), conj.neg());
  }

  // ((!a | !b) & (!c | !d) & (!e | !f))
  ConjunctiveFormula cnf = Flip(std::move(dnf));

  // ((!a & !c & !e) | ...)
  return DisjunctiveFormula::Create(pddl, std::move(cnf));
}

std::optional<DisjunctiveFormula> DisjunctiveFormula::Create(
    const Pddl& pddl, const VAL::goal* symbol,
    const std::vector<Object>& parameters,
    const std::vector<Object>& arguments) {
  // Proposition
  const auto* simple_goal = dynamic_cast<const VAL::simple_goal*>(symbol);
  if (simple_goal != nullptr) {
    const VAL::proposition* prop = simple_goal->getProp();
    const std::string name_predicate = prop->head->getName();
    const std::vector<Object> prop_params =
        Object::CreateList(pddl, prop->args);
    const auto Apply =
        Formula::CreateApplicationFunction(parameters, prop_params);
    return {
        {PartialState({Proposition(name_predicate, Apply(arguments))}, {})}};
    // return Simplify(pddl,
    //                 {{{Proposition(name_predicate, Apply(arguments))}, {}}});
  }

  // Conjunction
  const auto* conj_goal = dynamic_cast<const VAL::conj_goal*>(symbol);
  if (conj_goal != nullptr) {
    const VAL::goal_list* goals = conj_goal->getGoals();
    std::vector<DisjunctiveFormula> conj_terms;
    conj_terms.reserve(goals->size());
    for (const VAL::goal* goal : *goals) {
      std::optional<DisjunctiveFormula> conj =
          Create(pddl, goal, parameters, arguments);
      if (!conj.has_value()) return {};
      conj_terms.push_back(std::move(*conj));
    }
    return Conjoin(pddl, conj_terms);
  }

  // Disjunction
  const auto* disj_goal = dynamic_cast<const VAL::disj_goal*>(symbol);
  if (disj_goal != nullptr) {
    const VAL::goal_list* goals = disj_goal->getGoals();
    std::vector<DisjunctiveFormula> disj_terms;
    disj_terms.reserve(goals->size());
    for (const VAL::goal* goal : *goals) {
      std::optional<DisjunctiveFormula> disj =
          Create(pddl, goal, parameters, arguments);
      if (!disj.has_value()) continue;
      disj_terms.push_back(std::move(*disj));
    }
    return Disjoin(pddl, std::move(disj_terms));
  }

  // Negation
  const auto* neg_goal = dynamic_cast<const VAL::neg_goal*>(symbol);
  if (neg_goal != nullptr) {
    const VAL::goal* goal = neg_goal->getGoal();
    std::optional<DisjunctiveFormula> neg =
        Create(pddl, goal, parameters, arguments);
    if (!neg.has_value()) return DisjunctiveFormula();
    return Negate(pddl, std::move(*neg));
  }

  // Forall and exists
  const auto* qfied_goal = dynamic_cast<const VAL::qfied_goal*>(symbol);
  if (qfied_goal != nullptr) {
    const VAL::goal* goal = qfied_goal->getGoal();

    // Create qfied parameters
    std::vector<Object> qfied_params = parameters;
    std::vector<Object> types = Object::CreateList(pddl, qfied_goal->getVars());
    qfied_params.insert(qfied_params.end(), types.begin(), types.end());

    // Loop over qfied arguments
    std::vector<DisjunctiveFormula> qfied_terms;
    ParameterGenerator gen(pddl.object_map(), types);
    for (const std::vector<Object>& qfied_objs : gen) {
      // Create qfied arguments
      std::vector<Object> qfied_args = arguments;
      qfied_args.insert(qfied_args.end(), qfied_objs.begin(), qfied_objs.end());

      std::optional<DisjunctiveFormula> qfied =
          Create(pddl, goal, qfied_params, qfied_args);
      if (!qfied.has_value()) {
        switch (qfied_goal->getQuantifier()) {
          case VAL::quantifier::E_FORALL:
            return {};
          case VAL::quantifier::E_EXISTS:
            continue;
        }
      }
      qfied_terms.push_back(std::move(*qfied));
    }

    switch (qfied_goal->getQuantifier()) {
      case VAL::quantifier::E_FORALL:
        return Conjoin(pddl, qfied_terms);
      case VAL::quantifier::E_EXISTS:
        return Disjoin(pddl, std::move(qfied_terms));
    }
  }

  return {};
}

std::optional<DisjunctiveFormula> DisjunctiveFormula::Create(
    const Pddl& pddl, const VAL::effect_lists* symbol,
    const std::vector<Object>& parameters,
    const std::vector<Object>& arguments) {
  std::vector<DisjunctiveFormula> dnfs;

  // Forall effects
  for (const VAL::forall_effect* effect : symbol->forall_effects) {
    std::vector<Object> forall_params = parameters;
    const std::vector<Object> types =
        Object::CreateList(pddl, effect->getVarsList());
    forall_params.insert(forall_params.end(), types.begin(), types.end());

    // Loop over forall arguments
    ParameterGenerator gen(pddl.object_map(), types);
    for (const std::vector<Object>& forall_objs : gen) {
      // Create qfied arguments
      std::vector<Object> forall_args = arguments;
      forall_args.insert(forall_args.end(), forall_objs.begin(),
                         forall_objs.end());

      std::optional<DisjunctiveFormula> dnf =
          Create(pddl, effect->getEffects(), forall_params, forall_args);
      if (!dnf.has_value()) return {};
      dnfs.push_back(std::move(*dnf));
    }
  }

  // Add effects
  State pos;
  pos.reserve(symbol->add_effects.size());
  for (const VAL::simple_effect* effect : symbol->add_effects) {
    const std::string name_predicate = effect->prop->head->getName();
    const std::vector<Object> effect_params =
        Object::CreateList(pddl, effect->prop->args);
    const auto Apply =
        Formula::CreateApplicationFunction(parameters, effect_params);

    pos.emplace(name_predicate, Apply(arguments));
  }

  // Del effects
  State neg;
  neg.reserve(symbol->del_effects.size());
  for (const VAL::simple_effect* effect : symbol->del_effects) {
    const std::string name_predicate = effect->prop->head->getName();
    const std::vector<Object> effect_params =
        Object::CreateList(pddl, effect->prop->args);
    const auto Apply =
        Formula::CreateApplicationFunction(parameters, effect_params);

    neg.emplace(name_predicate, Apply(arguments));
  }

  if (!pos.empty() || !neg.empty()) {
    dnfs.push_back({PartialState(std::move(pos), std::move(neg))});
  }

  // Cond effects
  for (const VAL::cond_effect* effect : symbol->cond_effects) {
    std::optional<DisjunctiveFormula> condition =
        Create(pddl, effect->getCondition(), parameters, arguments);

    if (!condition.has_value()) continue;  // Condition always false

    std::optional<DisjunctiveFormula> result =
        Create(pddl, effect->getEffects(), parameters, arguments);

    if (condition->empty()) {
      // Condition always true
      if (!result.has_value()) {
        // Result always false
        throw std::runtime_error(
            "symbolic::DisjunctiveFormula::Create(): Invalid condition "
            "(context always true and result always false).");
      }
      dnfs.push_back(std::move(*result));
      continue;
    }

    condition = Negate(pddl, std::move(*condition));
    if (!condition.has_value()) continue;  // Axiom violated

    if (!result.has_value()) {
      // Result always false, so return neg condition
      dnfs.push_back(std::move(*condition));
      continue;
    }

    condition = Disjoin(pddl, {std::move(*condition), std::move(*result)});
    if (!condition.has_value()) {
      throw std::runtime_error(
          "DisjunctiveFormula::Create(): Invalid condition.");
    }
    dnfs.push_back(std::move(*condition));
  }

  return Conjoin(pddl, dnfs);
}

std::optional<std::pair<DisjunctiveFormula, DisjunctiveFormula>>
DisjunctiveFormula::NormalizeConditions(const Pddl& pddl,
                                        const std::string& action_call,
                                        bool apply_axioms) {
  const auto aa = Action::Parse(pddl, action_call);
  std::optional<DisjunctiveFormula> pre =
      DisjunctiveFormula::Create(pddl, aa.first.preconditions().symbol(),
                                 aa.first.parameters(), aa.second);
  if (!pre.has_value()) return {};
  std::optional<DisjunctiveFormula> post = DisjunctiveFormula::Create(
      pddl, aa.first.postconditions(), aa.first.parameters(), aa.second);
  if (!post.has_value()) return {};

  if (apply_axioms) {
    std::vector<DisjunctiveFormula::Conjunction> conj_pre;
    conj_pre.reserve(pre->conjunctions.size());
    for (const DisjunctiveFormula::Conjunction& conj : pre->conjunctions) {
      conj_pre.push_back(pddl.ConsistentState(conj));
    }
    std::vector<DisjunctiveFormula::Conjunction> conj_post;
    conj_post.reserve(pre->conjunctions.size());
    for (const DisjunctiveFormula::Conjunction& conj : post->conjunctions) {
      conj_post.push_back(pddl.ConsistentState(conj));
    }
    return std::make_pair(DisjunctiveFormula(std::move(conj_pre)),
                          DisjunctiveFormula(std::move(conj_post)));
  }
  return std::make_pair(std::move(*pre), std::move(*post));
}

std::optional<DisjunctiveFormula> DisjunctiveFormula::NormalizeGoal(
    const Pddl& pddl, bool apply_axioms) {
  std::optional<DisjunctiveFormula> goal =
      DisjunctiveFormula::Create(pddl, pddl.goal(), {}, {});
  if (!goal.has_value()) return goal;

  if (apply_axioms) {
    std::vector<DisjunctiveFormula::Conjunction> conj_goal;
    conj_goal.reserve(goal->conjunctions.size());
    for (const DisjunctiveFormula::Conjunction& conj : goal->conjunctions) {
      conj_goal.push_back(pddl.ConsistentState(conj));
    }
    goal = DisjunctiveFormula(std::move(conj_goal));
  }
  return goal;
}

std::ostream& operator<<(std::ostream& os, const DisjunctiveFormula& dnf) {
  os << "(or" << std::endl;
  for (const DisjunctiveFormula::Conjunction& conj : dnf.conjunctions) {
    os << "    (and" << std::endl;
    for (const Proposition& prop : conj.pos()) {
      os << "        " << prop << std::endl;
    }
    for (const Proposition& prop : conj.neg()) {
      os << "        not " << prop << std::endl;
    }
    os << "    )" << std::endl;
  }
  os << ")" << std::endl;
  return os;
}
std::ostream& operator<<(std::ostream& os, const ConjunctiveFormula& cnf) {
  os << "(and" << std::endl;
  for (const ConjunctiveFormula::Disjunction& disj : cnf.disjunctions) {
    os << "    (or" << std::endl;
    for (const Proposition& prop : disj.pos()) {
      os << "        " << prop << std::endl;
    }
    for (const Proposition& prop : disj.neg()) {
      os << "        not " << prop << std::endl;
    }
    os << "    )" << std::endl;
  }
  os << ")" << std::endl;
  return os;
}

}  // namespace symbolic_v1
