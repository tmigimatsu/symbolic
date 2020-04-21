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
#include <utility>  // std::rel_ops

namespace symbolic {

using namespace std::rel_ops;

Proposition::Proposition(const Pddl& pddl, const std::string& str_prop)
  : name_(ParseHead(str_prop)),
    arguments_(ParseArguments(pddl, str_prop)) {}

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

// bool operator<(const Proposition& lhs, const Proposition& rhs) {
//   if (lhs.name() != rhs.name()) return lhs.name() < rhs.name();
//   if (lhs.arguments().size() != rhs.arguments().size()) {
//     return lhs.arguments().size() < rhs.arguments().size();
//   }
//   for (size_t i = 0; i < lhs.arguments().size(); i++) {
//     if (lhs.arguments()[i] != rhs.arguments()[i]) return lhs.arguments()[i] < rhs.arguments()[i];
//   }
//   return false;
// }

// bool operator==(const Proposition& lhs, const Proposition& rhs) {
//   if (lhs.name() != rhs.name()) return false;
//   if (lhs.arguments().size() != rhs.arguments().size()) return false;
//   for (size_t i = 0; i < lhs.arguments().size(); i++) {
//     if (lhs.arguments()[i] != rhs.arguments()[i]) return false;
//   }
//   return true;
// }

}  // namespace symbolic
