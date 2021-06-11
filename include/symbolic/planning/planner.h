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

#include <functional>  // std::hash
#include <iostream>    // std::ostream
#include <memory>      // std::shared_ptr

#include "symbolic/pddl.h"

namespace symbolic {

class Planner {
 public:
  class Node {
    struct NodeImpl;

   public:
    class iterator;
    class reverse_iterator;

    Node() = default;
    Node(const Pddl& pddl, const State& state, size_t depth = 0);
    Node(const Node& parent, const Node& sibling, State&& state,
         std::string&& action);

    const std::string& action() const;
    const State& state() const;
    size_t depth() const;

    // Iterate over children
    iterator begin() const;
    iterator end() const;

    // Evaluate goal condition
    explicit operator bool() const;

    // Compare states for caching
    bool operator<(const Node& rhs) const;
    bool operator==(const Node& rhs) const;

    friend std::ostream& operator<<(std::ostream& os,
                                    const symbolic::Planner::Node& node);

   private:
    NodeImpl* operator->() { return impl_.get(); }
    const NodeImpl* operator->() const { return impl_.get(); }

    std::shared_ptr<NodeImpl> impl_;
  };

  /**
   * Planner class to find a state that satisfies the goal condition from the
   * initial state.
   *
   * @param pddl Pddl instance.
   *
   * @seepython{symbolic.Planner,__init__}
   */
  explicit Planner(const Pddl& pddl) : Planner(pddl, pddl.initial_state()) {}

  /**
   * Planner class to find a state that satisfies the goal condition from the
   * given state.
   *
   * @param pddl Pddl instance.
   * @param state State from which to search.
   *
   * @seepython{symbolic.Planner,__init__}
   */
  Planner(const Pddl& pddl, const State& state)
      : root_(pddl, pddl.ConsistentState(state)) {}

  const Node& root() const { return root_; }

 private:
  const Node root_;
};

class Planner::Node::iterator {
 public:
  using iterator_category = std::input_iterator_tag;
  using value_type = Node;
  using difference_type = ptrdiff_t;
  using pointer = const value_type*;
  using reference = const value_type&;

  explicit iterator(const Node& parent);

  iterator& operator++();
  iterator& operator--();
  bool operator==(const iterator& other) const;
  bool operator!=(const iterator& other) const { return !(*this == other); }
  reference operator*() const { return child_; }

 private:
  const Pddl& pddl_;

  const Node& parent_;
  Node child_;

  std::vector<Action>::const_iterator it_action_;
  ParameterGenerator::const_iterator it_param_;

  friend class Node;
};

}  // namespace symbolic

namespace std {

template <>
struct hash<symbolic::Planner::Node> {
  size_t operator()(const symbolic::Planner::Node& node) const noexcept;
};

}  // namespace std

#endif  // SYMBOLIC_PLANNING_PLANNER_H_
