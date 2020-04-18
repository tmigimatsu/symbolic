/**
 * normal_form.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: April 17, 2020
 * Authors: Toki Migimatsu
 */

#include "symbolic/normal_form.h"

#include <algorithm>  // std::swap
#include <iterator>   // std::make_move_iterator

#include "symbolic/pddl.h"
#include "symbolic/utils/combination_generator.h"

namespace symbolic {

DisjunctiveFormula Disjoin(std::vector<DisjunctiveFormula>&& dnfs) {
  DisjunctiveFormula disj;
  for (const DisjunctiveFormula& dnf : dnfs) {
    disj.conjunctions.insert(disj.conjunctions.end(),
                             std::make_move_iterator(dnf.conjunctions.begin()),
                             std::make_move_iterator(dnf.conjunctions.end()));
  }
  return disj;
}

DisjunctiveFormula Conjoin(const std::vector<DisjunctiveFormula>& dnfs) {
  // ((a | b) & (c | d) & (e | f))
  // ((a & c & e) | ...)
  DisjunctiveFormula conj;

  // Create combination generator for conjunctions
  std::vector<const std::vector<DisjunctiveFormula::Conjunction>*> conjunctions;
  conjunctions.reserve(dnfs.size());
  for (const DisjunctiveFormula& dnf : dnfs) {
    conjunctions.push_back(&dnf.conjunctions);
  }
  CombinationGenerator<const std::vector<DisjunctiveFormula::Conjunction>> gen(conjunctions);

  // Iterate over all combinations
  for (const std::vector<DisjunctiveFormula::Conjunction>& combo : gen) {
    DisjunctiveFormula::Conjunction term;
    for (const DisjunctiveFormula::Conjunction& term_i : combo) {
      term.pos.insert(term.pos.end(), term_i.pos.begin(), term_i.pos.end());
      term.neg.insert(term.neg.end(), term_i.neg.begin(), term_i.neg.end());
    }
    conj.conjunctions.push_back(std::move(term));
  }
  return conj;
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
    dnf.conjunctions.reserve(disj.pos.size() + disj.neg.size());
    for (Proposition& prop : disj.pos) {
      dnf.conjunctions.push_back({{ std::move(prop) }, {}});
    }
    for (Proposition& prop : disj.neg) {
      dnf.conjunctions.push_back({{}, { std::move(prop) }});
    }
    dnfs.push_back(std::move(dnf));
  }
  return dnfs;
}

DisjunctiveFormula::DisjunctiveFormula(ConjunctiveFormula&& cnf)
    : conjunctions(Conjoin(Convert(std::move(cnf))).conjunctions) {}

DisjunctiveFormula Negate(DisjunctiveFormula&& dnf) {
  // !((a & b) | (c & d) | (e & f))

  // ((!a & !b) | (!c & !d) | (!e & !f))
  for (DisjunctiveFormula::Conjunction& conj : dnf.conjunctions) {
    std::swap(conj.pos, conj.neg);
  }

  // ((!a | !b) & (!c | !d) & (!e | !f))
  ConjunctiveFormula cnf = Flip(std::move(dnf));

  // ((!a & !c & !e) | ...)
  return DisjunctiveFormula(std::move(cnf));
}

DisjunctiveFormula::DisjunctiveFormula(const Pddl& pddl, const VAL::goal* symbol,
                                       const std::vector<Object>& parameters,
                                       const std::vector<Object>& arguments) {

  // Proposition
  const VAL::simple_goal* simple_goal = dynamic_cast<const VAL::simple_goal*>(symbol);
  if (simple_goal != nullptr) {
    const VAL::proposition* prop = simple_goal->getProp();
    const std::string name_predicate = prop->head->getName();
    const std::vector<Object> prop_params = symbolic::ConvertObjects(prop->args);
    const auto Apply = CreateApplicationFunction(parameters, prop_params);

    conjunctions = {{{ Proposition(name_predicate, Apply(arguments)) }, {}}};
    return;
  }

  // Conjunction
  const VAL::conj_goal* conj_goal = dynamic_cast<const VAL::conj_goal*>(symbol);
  if (conj_goal != nullptr) {
    const VAL::goal_list* goals = conj_goal->getGoals();
    std::vector<DisjunctiveFormula> conj_terms;
    conj_terms.reserve(goals->size());
    for (const VAL::goal* goal : *goals) {
      conj_terms.emplace_back(pddl, goal, parameters, arguments);
    }
    conjunctions = Conjoin(conj_terms).conjunctions;
    return;
  }

  // Disjunction
  const VAL::disj_goal* disj_goal = dynamic_cast<const VAL::disj_goal*>(symbol);
  if (disj_goal != nullptr) {
    const VAL::goal_list* goals = disj_goal->getGoals();
    std::vector<DisjunctiveFormula> disj_terms;
    disj_terms.reserve(goals->size());
    for (const VAL::goal* goal : *goals) {
      disj_terms.emplace_back(pddl, goal, parameters, arguments);
    }
    conjunctions = Disjoin(std::move(disj_terms)).conjunctions;
    return;
  }

  // Negation
  const VAL::neg_goal* neg_goal = dynamic_cast<const VAL::neg_goal*>(symbol);
  if (neg_goal != nullptr) {
    const VAL::goal* goal = neg_goal->getGoal();
    conjunctions = Negate(DisjunctiveFormula(pddl, goal, parameters, arguments)).conjunctions;
    return;
  }

  // Forall and exists
  const VAL::qfied_goal* qfied_goal = dynamic_cast<const VAL::qfied_goal*>(symbol);
  if (qfied_goal != nullptr) {
    const VAL::goal* goal = qfied_goal->getGoal();

    // Create qfied parameters
    std::vector<Object> qfied_params = parameters;
    std::vector<Object> types = symbolic::ConvertObjects(qfied_goal->getVars());
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
        conjunctions = Conjoin(qfied_terms).conjunctions;
        return;
      case VAL::quantifier::E_EXISTS:
        conjunctions = Disjoin(std::move(qfied_terms)).conjunctions;
        return;
    }
  }
}

DisjunctiveFormula::DisjunctiveFormula(const Pddl& pddl, const VAL::effect_lists* effects,
                                       const std::vector<Object>& parameters,
                                       const std::vector<Object>& arguments) {
  std::vector<DisjunctiveFormula> dnfs;

  // Forall effects
  for (const VAL::forall_effect* effect : effects->forall_effects) {
    std::vector<Object> forall_params = parameters;
    const std::vector<Object> types = symbolic::ConvertObjects(effect->getVarsList());
    forall_params.insert(forall_params.end(), types.begin(), types.end());

    // Loop over forall arguments
    symbolic::ParameterGenerator gen(pddl.object_map(), types);
    for (const std::vector<Object>& forall_objs : gen) {
      // Create qfied arguments
      std::vector<Object> forall_args = arguments;
      forall_args.insert(forall_args.end(), forall_objs.begin(), forall_objs.end());

      dnfs.emplace_back(pddl, effect->getEffects(), forall_params, forall_args);
    }
  }

  // Add effects
  DisjunctiveFormula::Conjunction simple;
  simple.pos.reserve(effects->add_effects.size());
  for (const VAL::simple_effect* effect : effects->add_effects) {
    const std::string name_predicate = effect->prop->head->getName();
    const std::vector<Object> effect_params = symbolic::ConvertObjects(effect->prop->args);
    const auto Apply = CreateApplicationFunction(parameters, effect_params);

    simple.pos.emplace_back(name_predicate, Apply(arguments));
  }

  // Del effects
  simple.neg.reserve(effects->del_effects.size());
  for (const VAL::simple_effect* effect : effects->del_effects) {
    const std::string name_predicate = effect->prop->head->getName();
    const std::vector<Object> effect_params = symbolic::ConvertObjects(effect->prop->args);
    const auto Apply = CreateApplicationFunction(parameters, effect_params);

    simple.neg.emplace_back(name_predicate, Apply(arguments));
  }

  if (!simple.empty()) {
    dnfs.push_back({{ std::move(simple) }});
  }

  // Cond effects
  for (const VAL::cond_effect* effect : effects->cond_effects) {
    DisjunctiveFormula condition(pddl, effect->getCondition(), parameters, arguments);
    DisjunctiveFormula result(pddl, effect->getEffects(), parameters, arguments);
    dnfs.push_back(Disjoin({ Negate(std::move(condition)), std::move(result) }));
  }

  conjunctions = Conjoin(dnfs).conjunctions;
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
