/**
 * formula.h
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_FORMULA_H_
#define SYMBOLIC_FORMULA_H_

#include <functional>  // std::function
#include <set>         // std::set
#include <vector>      // std::vector

#include "ptree.h"

#include "symbolic/object.h"
#include "symbolic/proposition.h"
#include "symbolic/utils/parameter_generator.h"

namespace symbolic {

class Pddl;

class Action;

class Formula {

 public:

  Formula(const Pddl& pddl, const VAL::goal* symbol) : Formula(pddl, symbol, {}) {}

  Formula(const Pddl& pddl, const VAL::goal* symbol, const std::vector<Object>& parameters);

  Formula(const Pddl& pddl, const Action& action);

  const VAL::goal* symbol() const { return symbol_; }

  bool operator()(const std::set<Proposition>& state,
                  const std::vector<Object>& arguments) const { return P_(state, arguments); };

  bool operator()(const std::set<Proposition>& state) const { return P_(state, {}); };

 private:

  const VAL::goal* symbol_;

  std::function<bool(const std::set<Proposition>& state,
                     const std::vector<Object>& arguments)> P_;

};

std::function<std::vector<Object>(const std::vector<Object>&)>
CreateApplicationFunction(const std::vector<Object>& action_params,
                          const std::vector<Object>& prop_params);

}  // namespace symbolic

#endif  // SYMBOLIC_FORMULA_H_
