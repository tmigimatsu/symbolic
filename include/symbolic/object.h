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

  class Type {

   public:

    Type() {}

    Type(const VAL::pddl_type* symbol) : symbol_(symbol), name_(symbol->getName()) {}

    const VAL::pddl_type* symbol() const { return symbol_; }

    bool IsSubtype(const std::string& type) const;
    bool IsSubtype(const Type& type) const { return IsSubtype(type.name()); }

    std::vector<std::string> ListTypes() const;

    const std::string& name() const { return name_; }

   private:

    const VAL::pddl_type* symbol_ = nullptr;

    std::string name_;

  };

  Object() {}

  // Object(const VAL::pddl_typed_symbol* symbol) : symbol_(symbol), name_(symbol_->getName()) {}
  Object(const VAL::pddl_typed_symbol* symbol)
      : symbol_(symbol), name_(symbol->getName()), type_(symbol->type) {}

  Object(const Pddl& pddl, const std::string& name_object);

  // Object(const std::string& name_object, const std::string& type)
  //     : name_(name_object), type_(type) {}

  const VAL::pddl_typed_symbol* symbol() const { return symbol_; }

  const std::string& name() const { return name_; }

  const Type& type() const { return type_; }

 private:

  const VAL::pddl_typed_symbol* symbol_;

  std::string name_;
  Type type_;

};

// Atom is a proposition or action
std::vector<Object> ParseArguments(const Pddl& pddl, const std::string& atom);

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

inline std::ostream& operator<<(std::ostream& os, const Object::Type& type) {
  os << type.name();
  return os;
}

inline bool operator<(const Object::Type& lhs, const Object::Type& rhs) {
  return lhs.name() < rhs.name();
}

inline bool operator==(const Object::Type& lhs, const Object::Type& rhs) {
  return lhs.name() == rhs.name();
}

inline bool operator<(const Object& lhs, const Object& rhs) {
  return std::tie(lhs.name(), lhs.type()) < std::tie(rhs.name(), rhs.type());
}

inline bool operator==(const Object& lhs, const Object& rhs) {
  return std::tie(lhs.name(), lhs.type()) == std::tie(rhs.name(), rhs.type());
}
inline bool operator!=(const Object& lhs, const Object& rhs) {
  return !(lhs.type() == rhs.type());
}

}  // namespace symbolic

#endif  // SYMBOLIC_OBJECTS_H_
