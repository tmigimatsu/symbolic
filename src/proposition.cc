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

namespace {

constexpr size_t kHashOffset = 0x9e3779b9;
constexpr size_t kHashL = 6;
constexpr size_t kHashR = 2;

}  // namespace

namespace symbolic {

size_t PropositionBase::Hash(const PropositionBase& prop) {
  return Hash(prop, std::hash<std::string>{}(prop.name()));
}

size_t PropositionBase::Hash(const PropositionBase& prop,
                             size_t predicate_hash) {
  size_t seed = predicate_hash;
  for (const Object& arg : prop.arguments()) {
    seed ^= std::hash<Object>{}(arg) + kHashOffset + (seed << kHashL) +
            (seed >> kHashR);
  }
  return seed;
}

std::string PropositionBase::to_string() const {
  std::stringstream ss;
  ss << *this;
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, const PropositionBase& P) {
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
