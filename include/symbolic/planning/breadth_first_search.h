/**
 * breadth_first_search.h
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 29, 2018
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_PLANNING_BREADTH_FIRST_SEARCH_H_
#define SYMBOLIC_PLANNING_BREADTH_FIRST_SEARCH_H_

#include <cstddef>   // ptrdiff_t
#include <iterator>  // std::input_iterator_tag
#include <iostream>  // std::cout
#include <memory>    // std::shared_ptr
#include <queue>     // std::queue
#include <vector>    // std::vector
#include <utility>   // std::pair

namespace symbolic {

template<typename NodeT>
class BreadthFirstSearch {

 public:

  class iterator;

  BreadthFirstSearch(const NodeT& root, size_t max_depth) : kMaxDepth(max_depth), root_(root) {}

  iterator begin() { iterator it(root_, kMaxDepth); return ++it; }
  iterator end() { return iterator(); }

 private:

  const size_t kMaxDepth;

  const NodeT& root_;

};

template<typename NodeT>
class BreadthFirstSearch<NodeT>::iterator {

 public:

  using iterator_category = std::input_iterator_tag;
  using value_type = std::vector<NodeT>;
  using difference_type = ptrdiff_t;
  using pointer = const value_type*;
  using reference = const value_type&;

  using NodePtr = std::shared_ptr<const NodeT>;
  using NodePtrList = std::vector<NodePtr>;

  iterator() {}
  iterator(const NodeT& root, size_t max_depth)
      : queue_({{std::make_shared<NodeT>(root), std::make_shared<NodePtrList>()}}),
        kMaxDepth(max_depth) {}

  iterator& operator++();
  bool operator==(const iterator& other) const { return queue_.empty() && other.queue_.empty(); }
  bool operator!=(const iterator& other) const { return !(*this == other); }
  value_type operator*() const {
    plan_.clear();
    plan_.reserve(ancestors_->size());
    for (const NodePtr& node : *ancestors_) {
      plan_.push_back(*node);
    }
    return plan_;
  }

 private:

  const size_t kMaxDepth = 0;

  std::queue<std::pair<NodePtr, std::shared_ptr<NodePtrList>>> queue_;
  std::shared_ptr<NodePtrList> ancestors_;

  mutable std::vector<NodeT> plan_;

};

template<typename NodeT>
typename BreadthFirstSearch<NodeT>::iterator& BreadthFirstSearch<NodeT>::iterator::operator++() {
  size_t depth = 0;
  while (!queue_.empty()) {
    std::pair<NodePtr, std::shared_ptr<NodePtrList>>& front = queue_.front();

    // Take ancestors list and append current node
    ancestors_ = std::make_shared<NodePtrList>(*front.second);
    ancestors_->push_back(front.first);
    queue_.pop();

    // Print search depth
    if (ancestors_->size() > depth) {
      depth = ancestors_->size();
      std::cout << "BFS depth: " << depth - 1 << std::endl;
    }

    // Return if node evaluates to true
    const NodeT& node = *ancestors_->back();
    if (node) break;

    // Skip children if max depth has been reached
    if (ancestors_->size() > kMaxDepth) continue;

    // Add node's children to queue
    for (const NodeT& child : node) {
      queue_.emplace(std::make_shared<NodeT>(child), ancestors_);
    }
  }
  return *this;
}

}  // namespace symbolic

#endif  // SYMBOLIC_PLANNING_BREADTH_FIRST_SEARCH_H_
