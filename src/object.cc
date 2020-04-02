/**
 * objects.cc
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#include "symbolic/object.h"

#include <algorithm>  // std::min, std::replace
#include <cassert>    // assert
#include <sstream>    // std::stringstream

#include "symbolic/pddl.h"

/**
 * Create a map from object types to member objects.
 */
  /*
std::shared_ptr<const ObjectTypeMap> CreateObjectsMap(const VAL::const_symbol_list* constants,
                                                      const VAL::const_symbol_list* objects) {
  auto map_objects = std::make_shared<ObjectTypeMap>();

  // Iterate over constants
  for (const VAL::const_symbol* object : *constants) {
    const VAL::parameter_symbol* param = dynamic_cast<const VAL::parameter_symbol*>(object);

    // Iterate over this object's type superclasses
    for (const VAL::pddl_type* type = param->type; type != nullptr; type = type->type) {
      // Add object to the type's member vector'
      (*map_objects)[type].push_back(param);
    }
  }

  // Iterate over objects
  for (const VAL::const_symbol* object : *objects) {
    const VAL::parameter_symbol* param = dynamic_cast<const VAL::parameter_symbol*>(object);

    // Iterate over this object's type superclasses
    for (const VAL::pddl_type* type = param->type; type != nullptr; type = type->type) {
      // Add object to the type's member vector'
      (*map_objects)[type].push_back(param);
    }
  }
  return map_objects;
}
*/

namespace {

const VAL::const_symbol* GetSymbol(const symbolic::Pddl& pddl, const std::string& name_object) {
  assert(pddl.domain().constants != nullptr);
  for (const VAL::const_symbol* obj : *pddl.domain().constants) {
    assert(obj != nullptr);
    if (obj->getName() == name_object) return obj;
  }
  assert(pddl.problem().objects != nullptr);
  for (const VAL::const_symbol* obj : *pddl.problem().objects) {
    assert(obj != nullptr);
    if (obj->getName() == name_object) return obj;
  }
  throw std::runtime_error("Object::Object(): Could not find object symbol " + name_object + ".");
  return nullptr;
}

std::vector<std::string> TokenizeArguments(const std::string& proposition) {
  const size_t idx_start = proposition.find_first_of('(') + 1;
  const size_t idx_end = std::min(proposition.size(), proposition.find_last_of(')')) - idx_start;
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

Object::Object(const Pddl& pddl, const std::string& name_object)
    : symbol_(GetSymbol(pddl, name_object)),
      name_(name_object),
      type_(symbol_->type->getName()) {}

std::vector<Object> ParseArguments(const Pddl& pddl, const std::string& atom) {
  // std::cout << "ParseArguments: " << atom << " -> ";
  const std::vector<std::string> name_args = TokenizeArguments(atom);
  std::vector<Object> args;
  args.reserve(name_args.size());
  for (const std::string& name_arg : name_args) {
    // std::cout << name_arg << " ";
    args.emplace_back(pddl, name_arg);
  }
  // std::cout << std::endl;
  return args;
}

}  // namespace symbolic
