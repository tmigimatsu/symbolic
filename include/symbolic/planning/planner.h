/**
 * planner.h
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_PLANNING_PLANNER_H_
#define SYMBOLIC_PLANNING_PLANNER_H_

#include <memory>    // std::shared_ptr
#include <iostream>  // std::ostream
#include <set>       // std::ostream
#include <vector>    // std::vector

#include "ptree.h"

#include "symbolic/pddl.h"
#include "symbolic/utils/parameter_generator.h"

namespace symbolic {

class Planner {

 public:

  class Node {

   public:

    class iterator;
    class reverse_iterator;

    Node(const Pddl& pddl, size_t depth) : pddl_(pddl), depth_(depth) {}

    Node(const Pddl& pddl, const std::set<Proposition>& state, size_t depth = 0)
        : pddl_(pddl), state_(state), depth_(depth) {}

    const std::string action() const { return action_; }

    const std::set<Proposition>& state() const { return state_; }

    size_t depth() const { return depth_; }

    iterator begin() const;
    iterator end() const;

    explicit operator bool() const;

   private:

    const Pddl& pddl_;
    std::string action_;
    std::set<Proposition> state_;
    size_t depth_;

  };

  Planner(const Pddl& pddl);

  const Node& root() const { return root_; }

 private:

  const Pddl& pddl_;

  Node root_;

};

std::ostream& operator<<(std::ostream& os, const symbolic::Planner::Node& node);

class Planner::Node::iterator {

 public:

  using iterator_category = std::input_iterator_tag;
  using value_type = Node;
  using difference_type = ptrdiff_t;
  using pointer = const value_type*;
  using reference = const value_type&;

  iterator(const Node* parent);

  iterator& operator++();
  iterator& operator--();
  bool operator==(const iterator& other) const;
  bool operator!=(const iterator& other) const { return !(*this == other); }
  reference operator*() const { return child_; }

 private:

  const Pddl& pddl_;

  const Node* parent_;
  Node child_;

  std::set<Action>::const_iterator it_action_;

  ParameterGenerator param_gen_;
  ParameterGenerator::const_iterator it_param_;

  friend class Node;

};

}  // namespace symbolic

#endif  // SYMBOLIC_PLANNING_PLANNER_H_
