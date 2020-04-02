/**
 * planner.cc
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#include "symbolic/planning/planner.h"

#include "symbolic/action.h"

namespace symbolic {

Planner::Planner(const Pddl& pddl)
    : pddl_(pddl),
      root_(pddl_, pddl_.initial_state()) {}

std::ostream& operator<<(std::ostream& os, const symbolic::Planner::Node& node) {

  for (size_t i = 0; i < node.depth(); i++) {
    os << "-";
  }
  os << (node.depth() > 0 ? " " : "") << node.action() << " -> ";
  std::string separator;
  for (const Proposition& P : node.state()) {
    if (P.name() == "=") continue;
    os << separator << P;
    if (separator.empty()) separator = ", ";
  }

  return os;
}

Planner::Node::iterator::iterator(const Node* parent)
    : pddl_(parent->pddl_), parent_(parent), child_(pddl_, parent->depth_ + 1),
      it_action_(pddl_.actions().begin()),
      param_gen_(ParameterGenerator(pddl_.object_map(), it_action_->parameters())),
      it_param_(param_gen_.begin()) {}

Planner::Node::iterator& Planner::Node::iterator::operator++() {

  while (it_action_ != pddl_.actions().end()) {
    if (it_param_ == param_gen_.end()) {
      // Move onto next action
      ++it_action_;
      if (it_action_ == pddl_.actions().end()) break;
      const Action& action = *it_action_;

      // Generate new parameters
      param_gen_ = ParameterGenerator(pddl_.object_map(), action.parameters());
      it_param_ = param_gen_.begin();
    } else {
      // Move onto next parameters
      ++it_param_;
      if (it_param_ == param_gen_.end()) continue;
    }

    // Check action preconditions
    const Action& action = *it_action_;
    const std::vector<Object>& arguments = *it_param_;
    if (action.IsValid(parent_->state(), arguments)) {
      // Set action and apply postconditions to child
      child_.action_ = action.to_string(arguments);
      child_.state_ = action.Apply(parent_->state(), arguments);
      break;
    }
  }

  return *this;
}

Planner::Node::iterator& Planner::Node::iterator::operator--() {

  if (it_action_ == pddl_.actions().end()) {
    --it_action_;
    const Action& action = *it_action_;
    param_gen_ = ParameterGenerator(pddl_.object_map(), action.parameters());
    it_param_ = --param_gen_.end();

    // Check action preconditions
    const std::vector<Object>& arguments = *it_param_;
    if (action.IsValid(parent_->state(), arguments)) {
      // Set action and apply postconditions to child
      child_.action_ = action.to_string(arguments);
      child_.state_ = action.Apply(parent_->state(), arguments);
      return *this;
    }
  }

  while (it_action_ != pddl_.actions().begin() || it_param_ != param_gen_.begin()) {
    if (it_param_ == param_gen_.begin()) {
      // Move onto next action
      --it_action_;
      const Action& action = *it_action_;

      // Generate new parameters
      param_gen_ = ParameterGenerator(pddl_.object_map(), action.parameters());
      it_param_ = --param_gen_.end();
    } else {
      // Move onto next parameters
      --it_param_;
    }

    // Check action preconditions
    const Action& action = *it_action_;
    const std::vector<Object>& arguments = *it_param_;
    if (action.IsValid(parent_->state(), arguments)) {
      // Set action and apply postconditions to child
      child_.action_ = action.to_string(arguments);
      child_.state_ = action.Apply(parent_->state(), arguments);
      break;
    }
  }

  return *this;
}

bool Planner::Node::iterator::operator==(const iterator& other) const {
  return it_action_ == other.it_action_ && it_action_ == pddl_.actions().end();
}

Planner::Node::iterator Planner::Node::begin() const {
  iterator it(this);
  if (it == end()) return it;

  // Check action preconditions
  const Action& action = *it.it_action_;
  const std::vector<Object>& arguments = *it.it_param_;
  if (action.IsValid(it.parent_->state(), arguments)) {
    // Set action and apply postconditions to child
    it.child_.action_ = action.to_string(arguments);
    it.child_.state_ = action.Apply(state_, arguments);
    return it;
  }

  ++it;
  return it;
}

Planner::Node::iterator Planner::Node::end() const {
  iterator it(this);
  it.it_action_ = pddl_.actions().end();
  return it;
}

Planner::Node::operator bool() const {
  return pddl_.goal()(state_);
}

}  // namespace symbolic
