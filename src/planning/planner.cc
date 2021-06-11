/**
 * planner.cc
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#include "symbolic/planning/planner.h"

// #define SYMBOLIC_PLANNER_USE_ORDERED_CACHE

#ifdef SYMBOLIC_PLANNER_USE_ORDERED_CACHE
#include <set>            // std::set
#else                     // SYMBOLIC_PLANNER_USE_ORDERED_CACHE
#include <unordered_set>  // std::unordered_set
#endif                    // SYMBOLIC_PLANNER_USE_ORDERED_CACHE

namespace symbolic {

struct Planner::Node::NodeImpl {
#ifdef SYMBOLIC_PLANNER_USE_ORDERED_CACHE
  using Cache = std::set<Node>;
#else   // SYMBOLIC_PLANNER_USE_ORDERED_CACHE
  using Cache = std::unordered_set<Node>;
#endif  // SYMBOLIC_PLANNER_USE_ORDERED_CACHE

  NodeImpl(const Pddl& pddl, State&& state,
           const std::shared_ptr<const Cache>& ancestors, std::string&& action,
           size_t depth)
      : pddl_(pddl),
        state_(std::move(state)),
        ancestors_(ancestors),
        action_(std::move(action)),
        depth_(depth) {}

  NodeImpl(const Pddl& pddl, const State& state, size_t depth = 0)
      : pddl_(pddl),
        state_(state),
        ancestors_(std::make_shared<const Cache>()),
        depth_(depth) {}

  const Pddl& pddl_;

  const State state_;
  const std::shared_ptr<const Cache> ancestors_;

  // For debugging
  const std::string action_;
  const size_t depth_;
};

Planner::Node::Node(const Pddl& pddl, const State& state, size_t depth)
    : impl_(std::make_shared<NodeImpl>(pddl, state, depth)) {}

Planner::Node::Node(const Node& parent, const Node& sibling, State&& state,
                    std::string&& action) {
  if (!sibling.impl_) {
    auto ancestors =
        std::make_shared<Planner::Node::NodeImpl::Cache>(*parent->ancestors_);
    ancestors->insert(parent);
    impl_ = std::make_shared<NodeImpl>(parent->pddl_, std::move(state),
                                       std::move(ancestors), std::move(action),
                                       parent.depth() + 1);
  } else {
    impl_ = std::make_shared<NodeImpl>(sibling->pddl_, std::move(state),
                                       sibling->ancestors_, std::move(action),
                                       sibling.depth());
  }
}

const std::string& Planner::Node::action() const { return impl_->action_; }

const State& Planner::Node::state() const { return impl_->state_; }

size_t Planner::Node::depth() const { return impl_->depth_; }

Planner::Node::iterator Planner::Node::begin() const {
  iterator it(*this);
  if (it == end()) return it;

  if (it.it_param_ == it.it_action_->parameter_generator().end()) {
    ++it;
    return it;
  }

  // Check action preconditions
  const Action& action = *it.it_action_;
  const std::vector<Object>& arguments = *it.it_param_;
  const Node& parent = it.parent_;
  if (action.IsValid(parent.state(), arguments)) {
    // Set action and apply postconditions to child
    State state = action.Apply(parent.state(), arguments);
    DerivedPredicate::Apply(impl_->pddl_.derived_predicates(), &state);
    it.child_ =
        Node(parent, it.child_, std::move(state), action.to_string(arguments));

    // Return if state hasn't been previously visited
    const std::shared_ptr<const Planner::Node::NodeImpl::Cache> ancestors =
        it.child_->ancestors_;
    if (ancestors->count(it.child_) == 0) return it;
  }

  ++it;
  return it;
}

Planner::Node::iterator Planner::Node::end() const {
  iterator it(*this);
  it.it_action_ = impl_->pddl_.actions().end();
  return it;
}

Planner::Node::operator bool() const {
  return impl_->pddl_.goal()(impl_->state_);
}

bool Planner::Node::operator<(const Node& rhs) const {
  return impl_->state_ < rhs->state_;
}

bool Planner::Node::operator==(const Node& rhs) const {
  return impl_->state_ == rhs->state_;
}

std::ostream& bold_on(std::ostream& os) { return os << "\e[1m"; }
std::ostream& bold_off(std::ostream& os) { return os << "\e[0m"; }

std::ostream& operator<<(std::ostream& os, const Planner::Node& node) {
  os << bold_on;
  for (size_t i = 0; i < node.depth(); i++) {
    os << "-";
  }
  os << (node.depth() > 0 ? " " : "") << node.action() << " -> ";
  os << bold_off;
  os << node.state();

  return os;
}

Planner::Node::iterator::iterator(const Node& parent)
    : pddl_(parent->pddl_),
      parent_(parent),
      it_action_(pddl_.actions().begin()),
      it_param_(it_action_->parameter_generator().begin()) {}

Planner::Node::iterator& Planner::Node::iterator::operator++() {
  while (it_action_ != pddl_.actions().end()) {
    const ParameterGenerator& param_gen = it_action_->parameter_generator();
    if (it_param_ == param_gen.end()) {
      // Move onto next action
      ++it_action_;
      if (it_action_ == pddl_.actions().end()) break;

      // Generate new parameters
      it_param_ = it_action_->parameter_generator().begin();
      if (it_param_ == it_action_->parameter_generator().end()) continue;
    } else {
      // Move onto next parameters
      ++it_param_;
      if (it_param_ == param_gen.end()) continue;
    }

    // Check action preconditions
    const Action& action = *it_action_;
    const std::vector<Object>& arguments = *it_param_;
    if (action.IsValid(parent_.state(), arguments)) {
      // Set action and apply postconditions to child
      State state = action.Apply(parent_.state(), arguments);
      DerivedPredicate::Apply(pddl_.derived_predicates(), &state);
      child_ =
          Node(parent_, child_, std::move(state), action.to_string(arguments));

      // Return if state hasn't been previously visited
      const std::shared_ptr<const Planner::Node::NodeImpl::Cache> ancestors =
          child_->ancestors_;
      if (ancestors->count(child_) == 0) break;
    }
  }

  return *this;
}

Planner::Node::iterator& Planner::Node::iterator::operator--() {
  if (it_action_ == pddl_.actions().end()) {
    --it_action_;
    const Action& action = *it_action_;
    it_param_ = it_action_->parameter_generator().end();
    --it_param_;

    // Check action preconditions
    const std::vector<Object>& arguments = *it_param_;
    if (action.IsValid(parent_.state(), arguments)) {
      // Set action and apply postconditions to child
      State state = action.Apply(parent_.state(), arguments);
      DerivedPredicate::Apply(pddl_.derived_predicates(), &state);
      child_ =
          Node(parent_, child_, std::move(state), action.to_string(arguments));

      // Return if state hasn't been previously visited
      const std::shared_ptr<const Planner::Node::NodeImpl::Cache> ancestors =
          child_->ancestors_;
      if (ancestors->count(child_) == 0) return *this;
    }
  }

  while (it_action_ != pddl_.actions().begin() ||
         it_param_ != it_action_->parameter_generator().begin()) {
    if (it_param_ == it_action_->parameter_generator().begin()) {
      // Move onto next action
      --it_action_;

      // Generate new parameters
      it_param_ = it_action_->parameter_generator().end();
      --it_param_;
    } else {
      // Move onto next parameters
      --it_param_;
    }

    // Check action preconditions
    const Action& action = *it_action_;
    const std::vector<Object>& arguments = *it_param_;
    if (action.IsValid(parent_.state(), arguments)) {
      // Set action and apply postconditions to child
      State state = action.Apply(parent_.state(), arguments);
      DerivedPredicate::Apply(pddl_.derived_predicates(), &state);
      child_ =
          Node(parent_, child_, std::move(state), action.to_string(arguments));

      // Return if state hasn't been previously visited
      const std::shared_ptr<const Planner::Node::NodeImpl::Cache> ancestors =
          child_->ancestors_;
      if (ancestors->count(child_) == 0) break;
    }
  }

  return *this;
}

bool Planner::Node::iterator::operator==(const iterator& other) const {
  return it_action_ == other.it_action_ && it_action_ == pddl_.actions().end();
}

}  // namespace symbolic

namespace std {

size_t hash<::symbolic::Planner::Node>::operator()(
    const ::symbolic::Planner::Node& node) const noexcept {
  return hash<::symbolic::State>{}(node.state());
}

}  // namespace std
