/**
 * predicate.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: June 15, 2020
 * Authors: Toki Migimatsu
 */

#include "symbolic/predicate.h"

#include <VAL/ptree.h>

#include <sstream>  // std::stringstream

#include "symbolic/pddl.h"

namespace symbolic {

Predicate::Predicate(const Pddl& pddl, const VAL::pred_decl* symbol)
    : Action(pddl),
      symbol_(symbol),
      name_(symbol_->getPred()->getName()),
      parameters_(Object::CreateList(pddl, symbol_->getArgs())),
      param_gen_(ParameterGenerator(pddl, parameters_)) {}

std::string Predicate::to_string() const {
  std::stringstream ss;
  ss << *this;
  return ss.str();
}

std::string Predicate::to_string(const std::vector<Object>& arguments) const {
  std::stringstream ss;
  ss << name_ << "(";
  std::string separator;
  for (const Object& arg : arguments) {
    ss << separator << arg;
    if (separator.empty()) separator = ", ";
  }
  ss << ")";
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, const Predicate& pred) {
  os << pred.name() << "(";
  std::string separator;
  for (const Object& param : pred.parameters()) {
    os << separator << param;
    if (separator.empty()) separator = ", ";
  }
  os << ")";
  return os;
}

}  // namespace symbolic
