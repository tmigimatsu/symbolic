/**
 * proposition.cc
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#include "symbolic/proposition.h"

#include <sstream>  // std::stringstream

namespace symbolic {

Proposition::Proposition(const Pddl& pddl, const std::string& str_prop)
    : name_(ParseHead(str_prop)), arguments_(Object::ParseArguments(pddl, str_prop)) {}

Proposition::Proposition(const std::string& str_prop)
    : name_(ParseHead(str_prop)), arguments_(Object::ParseArguments(str_prop)) {}

std::string Proposition::to_string() const {
  std::stringstream ss;
  ss << *this;
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, const symbolic::Proposition& P) {
  os << P.name() << "(";
  std::string separator;
  for (const Object& arg : P.arguments()) {
    os << separator << arg;
    if (separator.empty()) separator = ", ";
  }
  os << ")";
  return os;
}

}  // namespace symbolic
