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
  size_t seed = std::hash<std::string>{}(prop.name());
  for (const symbolic::Object& arg : prop.arguments()) {
    seed ^= std::hash<std::string>{}(arg.name()) + kHashOffset +
            (seed << kHashL) + (seed >> kHashR);
  }
  return seed;
}

std::string PropositionBase::to_string() const {
  std::stringstream ss;
  ss << *this;
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, const symbolic::PropositionBase& P) {
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

namespace std {

size_t hash<symbolic::PropositionBase>::operator()(
    const symbolic::PropositionBase& prop) const noexcept {
  return prop.hash();
}

size_t hash<symbolic::Proposition>::operator()(
    const symbolic::Proposition& prop) const noexcept {
  return prop.hash();
}

size_t hash<symbolic::PropositionRef>::operator()(
    const symbolic::PropositionRef& prop) const noexcept {
  return prop.hash();
}

};  // namespace std
