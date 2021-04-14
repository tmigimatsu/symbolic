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

#include <memory>   // std::shared_ptr
#include <ostream>  // std::ostream
#include <tuple>    // std::tie
#include <vector>   // std::vector

namespace VAL {

class pddl_type;
class pddl_typed_symbol;

template <typename T>
class typed_symbol_list;

using pddl_type_list = class typed_symbol_list<pddl_type>;

}  // namespace VAL

namespace symbolic {

class Pddl;

class Object {
 public:
  class Type {
   public:
    Type() = default;

    explicit Type(const VAL::pddl_type* symbol);

    const VAL::pddl_type* symbol() const { return symbol_; }

    bool IsSubtype(const std::string& type) const;
    bool IsSubtype(const Type& type) const { return IsSubtype(type.name()); }

    std::vector<std::string> ListTypes() const;

    const std::string& name() const { return *name_; }

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

    const std::string* name_;
  };

  Object() = default;

  Object(const Pddl& pddl, const VAL::pddl_typed_symbol* symbol);

  Object(const VAL::pddl_type_list* types,
         const VAL::pddl_typed_symbol* symbol);

  Object(const Pddl& pddl, const std::string& name_object);

  explicit Object(const std::string& name_object);

  const VAL::pddl_typed_symbol* symbol() const { return symbol_; }

  const std::string& name() const { return *name_; }

  const Type& type() const { return type_; }

  // Atom is a proposition or action
  static std::vector<Object> ParseArguments(const Pddl& pddl,
                                            const std::string& atom);

  static std::vector<Object> ParseArguments(const std::string& atom);

  static std::vector<Object> ParseArguments(
      const std::vector<std::string>& str_args);

  template <typename T>
  static std::vector<Object> CreateList(
      const Pddl& pddl, const VAL::typed_symbol_list<T>* symbols);

  template <typename T>
  static std::vector<Object> CreateList(
      const VAL::pddl_type_list* types,
      const VAL::typed_symbol_list<T>* symbols);

  friend bool operator<(const Object& lhs, const Object& rhs) {
    return &lhs.name() != &rhs.name() && lhs.name() < rhs.name();
    // return std::tie(lhs.name(), lhs.type()) < std::tie(rhs.name(),
    // rhs.type());
  }

  friend bool operator==(const Object& lhs, const Object& rhs) {
    return &lhs.name() == &rhs.name() || lhs.name() == rhs.name();
    // return std::tie(lhs.name(), lhs.type()) == std::tie(rhs.name(),
    // rhs.type());
  }
  friend bool operator!=(const Object& lhs, const Object& rhs) {
    return lhs.name() != rhs.name();
  }

  friend std::ostream& operator<<(std::ostream& os, const Object& object) {
    os << object.name();
    return os;
  }

 private:
  const VAL::pddl_typed_symbol* symbol_ = nullptr;

  std::shared_ptr<std::string> name_storage_;
  const std::string* name_;
  Type type_;
};

template <typename T>
std::vector<Object> Object::CreateList(
    const Pddl& pddl, const VAL::typed_symbol_list<T>* symbols) {
  std::vector<Object> objects;
  objects.reserve(symbols->size());
  for (const T* symbol : *symbols) {
    objects.emplace_back(pddl, symbol);
  }
  return objects;
}

template <typename T>
std::vector<Object> Object::CreateList(
    const VAL::pddl_type_list* types,
    const VAL::typed_symbol_list<T>* symbols) {
  std::vector<Object> objects;
  objects.reserve(symbols->size());
  for (const T* symbol : *symbols) {
    objects.emplace_back(types, symbol);
  }
  return objects;
}

std::ostream& operator<<(std::ostream& os, const std::vector<Object>& objects);

}  // namespace symbolic

#endif  // SYMBOLIC_OBJECTS_H_
