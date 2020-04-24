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

#include <VAL/ptree.h>

#include <ostream>  // std::ostream
#include <tuple>    // std::tie
#include <vector>   // std::vector

namespace symbolic {

class Pddl;

class Object {
 public:
  class Type {
   public:
    Type() {}

    Type(const VAL::pddl_type* symbol)
        : symbol_(symbol), name_(symbol->getName()) {}

    const VAL::pddl_type* symbol() const { return symbol_; }

    bool IsSubtype(const std::string& type) const;
    bool IsSubtype(const Type& type) const { return IsSubtype(type.name()); }

    std::vector<std::string> ListTypes() const;

    const std::string& name() const { return name_; }

    friend bool operator<(const Object::Type& lhs, const Object::Type& rhs) {
      return lhs.name() < rhs.name();
    }

    friend bool operator==(const Object::Type& lhs, const Object::Type& rhs) {
      return lhs.name() == rhs.name();
    }

    friend std::ostream& operator<<(std::ostream& os,
                                    const Object::Type& type) {
      os << type.name();
      return os;
    }

   private:
    const VAL::pddl_type* symbol_ = nullptr;

    std::string name_;
  };

  Object() {}

  Object(const Pddl& pddl, const VAL::pddl_typed_symbol* symbol);

  Object(const VAL::pddl_type_list* types,
         const VAL::pddl_typed_symbol* symbol);

  Object(const Pddl& pddl, const std::string& name_object);

  const VAL::pddl_typed_symbol* symbol() const { return symbol_; }

  const std::string& name() const { return name_; }

  const Type& type() const { return type_; }

  friend bool operator<(const Object& lhs, const Object& rhs) {
    return std::tie(lhs.name(), lhs.type()) < std::tie(rhs.name(), rhs.type());
  }

  friend bool operator==(const Object& lhs, const Object& rhs) {
    return std::tie(lhs.name(), lhs.type()) == std::tie(rhs.name(), rhs.type());
  }
  friend bool operator!=(const Object& lhs, const Object& rhs) {
    return !(lhs.type() == rhs.type());
  }

  friend std::ostream& operator<<(std::ostream& os, const Object& object) {
    os << object.name();
    return os;
  }

 private:
  const VAL::pddl_typed_symbol* symbol_;

  std::string name_;
  Type type_;
};

// Atom is a proposition or action
std::vector<Object> ParseArguments(const Pddl& pddl, const std::string& atom);

template <typename T>
std::vector<Object> ConvertObjects(const Pddl& pddl,
                                   const VAL::typed_symbol_list<T>* symbols) {
  std::vector<Object> objects;
  objects.reserve(symbols->size());
  for (const T* symbol : *symbols) {
    objects.emplace_back(pddl, symbol);
  }
  return objects;
}

template <typename T>
std::vector<Object> ConvertObjects(const VAL::pddl_type_list* types,
                                   const VAL::typed_symbol_list<T>* symbols) {
  std::vector<Object> objects;
  objects.reserve(symbols->size());
  for (const T* symbol : *symbols) {
    objects.emplace_back(types, symbol);
  }
  return objects;
}

}  // namespace symbolic

#endif  // SYMBOLIC_OBJECTS_H_
