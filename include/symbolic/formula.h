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
#include <optional>    // std::optional
#include <ostream>     // std::ostream
#include <set>         // std::set
#include <vector>      // std::vector

#include "symbolic/object.h"
#include "symbolic/proposition.h"
#include "symbolic/state.h"
#include "symbolic/utils/parameter_generator.h"

namespace VAL {

class goal;

}  // namespace VAL

namespace symbolic {

class Pddl;

class Action;

class Formula {
 public:
  Formula() = default;

  Formula(const Pddl& pddl, const VAL::goal* symbol)
      : Formula(pddl, symbol, {}) {}

  Formula(const Pddl& pddl, const VAL::goal* symbol,
          const std::vector<Object>& parameters);

  const VAL::goal* symbol() const { return symbol_; }

  bool operator()(const State& state,
                  const std::vector<Object>& arguments) const {
    return P_(state, arguments);
  };

  bool operator()(const State& state) const { return P_(state, {}); };

  std::optional<bool> operator()(const PartialState& state,
                                 const std::vector<Object>& arguments) const;

  std::optional<bool> operator()(const PartialState& state) const;

  const std::string& to_string() const { return str_formula_; }

  friend std::ostream& operator<<(std::ostream& os, const Formula& F);

  /**
   * Creates a function that takes action_args and returns prop_args based on the
   * mapping (action_params -> prop_params).
   */
  static std::function<const std::vector<Object>&(const std::vector<Object>&)>
  CreateApplicationFunction(const std::vector<Object>& action_params,
                            const std::vector<Object>& prop_params);

 private:
  const VAL::goal* symbol_ = nullptr;

  std::function<bool(const State& state, const std::vector<Object>& arguments)>
      P_;

  std::function<bool(const PartialState& state,
                     const std::vector<Object>& arguments)>
      PP_;

  std::string str_formula_;
};

}  // namespace symbolic

#endif  // SYMBOLIC_FORMULA_H_
