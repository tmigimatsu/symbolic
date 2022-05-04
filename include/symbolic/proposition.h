/**
 * proposition.h
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_PROPOSITION_H_
#define SYMBOLIC_PROPOSITION_H_

#include <functional>  // std::equal_to, std::hash
#include <ostream>     // std::ostream
#include <string>      // std::string
#include <utility>     // std::tie
#include <vector>      // std::vector

#include "symbolic/object.h"

namespace symbolic {

class Pddl;

class PropositionBase {
 public:
  virtual const std::string& name() const = 0;

  virtual const std::vector<Object>& arguments() const = 0;

  virtual std::string to_string() const;
  virtual std::string to_pddl() const;

  size_t hash() const { return hash_; }

  static std::string ParseHead(const std::string& atom) {
    return atom.substr(0, atom.find_first_of('('));
  }

  friend bool operator<(const PropositionBase& lhs,
                        const PropositionBase& rhs) {
    return std::tie(lhs.name(), lhs.arguments()) <
           std::tie(rhs.name(), rhs.arguments());
  }
  friend bool operator==(const PropositionBase& lhs,
                         const PropositionBase& rhs) {
    return std::tie(lhs.hash_, lhs.name(), lhs.arguments()) ==
           std::tie(rhs.hash_, rhs.name(), rhs.arguments());
  }
  friend bool operator!=(const PropositionBase& lhs,
                         const PropositionBase& rhs) {
    return !(lhs == rhs);
  }

  friend std::ostream& operator<<(std::ostream& os, const PropositionBase& P);

 protected:
  static size_t Hash(const PropositionBase& prop);
  static size_t Hash(const PropositionBase& prop, size_t predicate_hash);

  void PrecomputeHash() { hash_ = Hash(*this); }
  void PrecomputeHash(size_t predicate_hash) { hash_ = Hash(*this, predicate_hash); }

  size_t hash_;
};

class Proposition : public PropositionBase {
 public:
  Proposition() = default;

  Proposition(const std::string& name_predicate,
              std::vector<Object>&& arguments)
      : name_(name_predicate), arguments_(std::move(arguments)) {
    PrecomputeHash();
  }

  Proposition(const std::string& name_predicate,
              const std::vector<Object>& arguments)
      : name_(name_predicate), arguments_(arguments) {
    PrecomputeHash();
  }

  Proposition(const Pddl& pddl, const std::string& str_prop)
      : name_(ParseHead(str_prop)),
        arguments_(Object::ParseArguments(pddl, str_prop)) {
    PrecomputeHash();
  }

  // explicit Proposition(const std::string& str_prop)
  //     : name_(ParseHead(str_prop)),
  //       arguments_(Object::ParseArguments(str_prop)) {
  //   PrecomputeHash();
  // }

  explicit Proposition(const PropositionBase& other)
      : PropositionBase(other),
        name_(other.name()),
        arguments_(other.arguments()) {}

  const std::string& name() const override { return name_; }

  const std::vector<Object>& arguments() const override { return arguments_; }

 private:
  std::string name_;
  std::vector<Object> arguments_;
};

class PropositionRef : public PropositionBase {
 public:
  PropositionRef(const std::string* name_predicate,
                 const std::vector<Object>* arguments,
                 size_t predicate_hash)
      : name_(name_predicate), arguments_(arguments) {
    PrecomputeHash(predicate_hash);
  }

  const std::string& name() const override { return *name_; }

  const std::vector<Object>& arguments() const override { return *arguments_; }

 private:
  const std::string* name_;
  const std::vector<Object>* arguments_;
};

class SignedProposition : public Proposition {
 public:
  SignedProposition(Proposition&& prop, bool is_pos)
      : Proposition(std::move(prop)), is_pos_(is_pos) {}

  SignedProposition(const std::string& name_predicate,
                    std::vector<Object>&& arguments, bool is_pos)
      : Proposition(name_predicate, std::move(arguments)), is_pos_(is_pos) {}

  bool is_pos() const { return is_pos_; }

  std::string sign() const { return Sign(is_pos()); };

  static std::string Sign(bool is_pos) { return is_pos ? "+" : "-"; };

 private:
  bool is_pos_;
};

}  // namespace symbolic

namespace std {

template <>
struct hash<symbolic::PropositionBase> {
  using is_transparent = void;
  size_t operator()(const symbolic::PropositionBase& prop) const noexcept {
    return prop.hash();
  }
};

template <>
struct hash<symbolic::Proposition> {
  using is_transparent = void;
  size_t operator()(const symbolic::Proposition& prop) const noexcept {
    return prop.hash();
  }
};

template <>
struct hash<symbolic::PropositionRef> {
  using is_transparent = void;
  size_t operator()(const symbolic::PropositionRef& prop) const noexcept {
    return prop.hash();
  }
};

}  // namespace std

#endif  // SYMBOLIC_PROPOSITION_H_
