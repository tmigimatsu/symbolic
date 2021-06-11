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

#include <ostream>  // std::ostream
#include <tuple>    // std::tie
#include <vector>   // std::vector

namespace VAL_v1 {

class pddl_type;
class pddl_typed_symbol;

template<typename T>
class typed_symbol_list;

using pddl_type_list = class typed_symbol_list<pddl_type>;

}  // namespace VAL_v1

namespace symbolic_v1 {

class Pddl;

class Object {
 public:
  class Type {
   public:
    Type() = default;

    explicit Type(const VAL_v1::pddl_type* symbol);

    const VAL_v1::pddl_type* symbol() const { return symbol_; }

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
    const VAL_v1::pddl_type* symbol_ = nullptr;

    std::string name_;
  };

  Object() = default;

  Object(const Pddl& pddl, const VAL_v1::pddl_typed_symbol* symbol);

  Object(const VAL_v1::pddl_type_list* types,
         const VAL_v1::pddl_typed_symbol* symbol);

  Object(const Pddl& pddl, const std::string& name_object);

  explicit Object(const std::string& name_object) : name_(name_object){};

  const VAL_v1::pddl_typed_symbol* symbol() const { return symbol_; }

  const std::string& name() const { return name_; }

  const Type& type() const { return type_; }

  // Atom is a proposition or action
  static std::vector<Object> ParseArguments(const Pddl& pddl,
                                            const std::string& atom);

  static std::vector<Object> ParseArguments(const std::string& atom);

  static std::vector<Object> ParseArguments(
      const std::vector<std::string>& str_args);

  template <typename T>
  static std::vector<Object> CreateList(
      const Pddl& pddl, const VAL_v1::typed_symbol_list<T>* symbols);

  template <typename T>
  static std::vector<Object> CreateList(
      const VAL_v1::pddl_type_list* types,
      const VAL_v1::typed_symbol_list<T>* symbols);

  friend bool operator<(const Object& lhs, const Object& rhs) {
    return lhs.name() < rhs.name();
    // return std::tie(lhs.name(), lhs.type()) < std::tie(rhs.name(),
    // rhs.type());
  }

  friend bool operator==(const Object& lhs, const Object& rhs) {
    return lhs.name() == rhs.name();
    // return std::tie(lhs.name(), lhs.type()) == std::tie(rhs.name(),
    // rhs.type());
  }
  friend bool operator!=(const Object& lhs, const Object& rhs) {
    return !(lhs == rhs);
  }

  friend std::ostream& operator<<(std::ostream& os, const Object& object) {
    os << object.name();
    return os;
  }

 private:
  const VAL_v1::pddl_typed_symbol* symbol_ = nullptr;

  std::string name_;
  Type type_;
};

template <typename T>
std::vector<Object> Object::CreateList(
    const Pddl& pddl, const VAL_v1::typed_symbol_list<T>* symbols) {
  std::vector<Object> objects;
  objects.reserve(symbols->size());
  for (const T* symbol : *symbols) {
    objects.emplace_back(pddl, symbol);
  }
  return objects;
}

template <typename T>
std::vector<Object> Object::CreateList(
    const VAL_v1::pddl_type_list* types,
    const VAL_v1::typed_symbol_list<T>* symbols) {
  std::vector<Object> objects;
  objects.reserve(symbols->size());
  for (const T* symbol : *symbols) {
    objects.emplace_back(types, symbol);
  }
  return objects;
}

}  // namespace symbolic_v1

#endif  // SYMBOLIC_OBJECTS_H_
