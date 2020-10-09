/**
 * objects.cc
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#include "symbolic/object.h"

#include <VAL/ptree.h>

#include <algorithm>  // std::min, std::replace
#include <cassert>    // assert
#include <sstream>    // std::stringstream

#include "symbolic/pddl.h"

namespace {

const VAL::pddl_type* GetTypeSymbol(const VAL::pddl_type_list* types,
                                    const VAL::pddl_type* symbol = nullptr) {
  if (symbol != nullptr) return symbol;
  // Iterate over domain types
  for (const VAL::pddl_type* type : *types) {
    // Iterate over ancestors of current type
    for (; type != nullptr; type = type->type) {
      if (type->type == nullptr && type->getName() == "object") return type;
    }
  }
  return nullptr;
}

const VAL::const_symbol* GetSymbol(const symbolic::Pddl& pddl,
                                   const std::string& name_object) {
  assert(pddl.symbol()->the_domain->constants != nullptr);
  for (const VAL::const_symbol* obj : *pddl.symbol()->the_domain->constants) {
    assert(obj != nullptr);
    if (obj->getName() == name_object) return obj;
  }
  assert(pddl.symbol()->the_problem->objects != nullptr);
  for (const VAL::const_symbol* obj : *pddl.symbol()->the_problem->objects) {
    assert(obj != nullptr);
    if (obj->getName() == name_object) return obj;
  }
  throw std::runtime_error("Object::Object(): Could not find object symbol " +
                           name_object + ".");
  return nullptr;
}

std::vector<std::string> TokenizeArguments(const std::string& proposition) {
  const size_t idx_start = proposition.find_first_of('(') + 1;
  const size_t idx_end =
      std::min(proposition.size(), proposition.find_last_of(')')) - idx_start;
  std::string str_args = proposition.substr(idx_start, idx_end);
  std::replace(str_args.begin(), str_args.end(), ',', ' ');
  std::stringstream ss(str_args);

  std::string arg;
  std::vector<std::string> args;
  while (ss >> arg) {
    args.emplace_back(std::move(arg));
  }
  return args;
}

}  // namespace

namespace symbolic {

Object::Type::Type(const VAL::pddl_type* symbol)
    : symbol_(symbol), name_(symbol->getName()) {}

bool Object::Type::IsSubtype(const std::string& type) const {
  for (const VAL::pddl_type* curr = symbol_; curr != nullptr;
       curr = curr->type) {
    if (curr->getName() == type) return true;
  }
  return false;
}

std::vector<std::string> Object::Type::ListTypes() const {
  std::vector<std::string> types;
  for (const VAL::pddl_type* curr = symbol_; curr != nullptr;
       curr = curr->type) {
    types.push_back(curr->getName());
  }
  return types;
}

Object::Object(const Pddl& pddl, const VAL::pddl_typed_symbol* symbol)
    : symbol_(symbol),
      name_(symbol->getName()),
      type_(GetTypeSymbol(pddl.symbol()->the_domain->types, symbol->type)) {}

Object::Object(const VAL::pddl_type_list* types,
               const VAL::pddl_typed_symbol* symbol)
    : symbol_(symbol),
      name_(symbol->getName()),
      type_(GetTypeSymbol(types, symbol->type)) {}

Object::Object(const Pddl& pddl, const std::string& name_object)
    : symbol_(GetSymbol(pddl, name_object)),
      name_(name_object),
      type_(GetTypeSymbol(pddl.symbol()->the_domain->types, symbol_->type)) {}

std::vector<Object> Object::ParseArguments(const Pddl& pddl,
                                           const std::string& atom) {
  const std::vector<std::string> name_args = TokenizeArguments(atom);
  std::vector<Object> args;
  args.reserve(name_args.size());
  for (const std::string& name_arg : name_args) {
    args.emplace_back(pddl, name_arg);
  }
  return args;
}

std::vector<Object> Object::ParseArguments(const std::string& atom) {
  const std::vector<std::string> name_args = TokenizeArguments(atom);
  return ParseArguments(name_args);
}

std::vector<Object> Object::ParseArguments(
    const std::vector<std::string>& str_args) {
  std::vector<Object> args;
  args.reserve(str_args.size());
  for (const std::string& str_arg : str_args) {
    args.emplace_back(str_arg);
  }
  return args;
}

}  // namespace symbolic
