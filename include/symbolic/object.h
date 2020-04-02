/**
 * object.h
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_OBJECTS_H_
#define SYMBOLIC_OBJECTS_H_

#include <map>      // std::map
#include <memory>   // std::shared_ptr
#include <vector>   // std::vector
#include <tuple>    // std::tie
#include <ostream>  // std::ostream

#include "ptree.h"

namespace symbolic {

class Pddl;

class Object {

 public:

  Object() {}

  // Object(const VAL::pddl_typed_symbol* symbol) : symbol_(symbol), name_(symbol_->getName()) {}
  Object(const VAL::pddl_typed_symbol* symbol)
      : symbol_(symbol), name_(symbol->getName()), type_(symbol->type->getName()) {}

  Object(const Pddl& pddl, const std::string& name_object);

  Object(const std::string& name_object, const std::string& type)
      : name_(name_object), type_(type) {}

  const VAL::pddl_typed_symbol* symbol() const { return symbol_; }

  const std::string& name() const { return name_; }

  const std::string& type() const { return type_; }

  // bool operator<(const Object& rhs) const;
  // bool operator==(const Object& rhs) const;
  // bool operator!=(const Object& rhs) const;

 private:

  const VAL::pddl_typed_symbol* symbol_;
  // TODO: Make const?
  std::string name_;
  std::string type_;

};

// Atom is a proposition or action
std::vector<Object> ParseArguments(const Pddl& pddl, const std::string& atom);

// using ObjectTypeMap = std::map<std::string, std::vector<Object>>;

// std::shared_ptr<const ObjectTypeMap> CreateObjectsMap(const VAL::const_symbol_list* constants,
//                                                       const VAL::const_symbol_list* objects);


template<typename T>
std::vector<Object> ConvertObjects(const VAL::typed_symbol_list<T>* symbols) {
  std::vector<Object> objects;
  objects.reserve(symbols->size());
  for (const T* symbol : *symbols) {
    objects.emplace_back(symbol);
  }
  return objects;
}

inline std::ostream& operator<<(std::ostream& os, const Object& object) {
  os << object.name();
  return os;
}

inline bool operator<(const Object& lhs, const Object& rhs) {
  return std::tie(lhs.type(), lhs.name()) < std::tie(rhs.type(), rhs.name());
}

inline bool operator==(const Object& lhs, const Object& rhs) {
  return lhs.type() == rhs.type() && lhs.name() == rhs.name();
}

}  // namespace symbolic

#endif  // SYMBOLIC_OBJECTS_H_
