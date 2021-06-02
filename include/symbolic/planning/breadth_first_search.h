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

#include <chrono>    // std::chrono
#include <cstddef>   // ptrdiff_t
#include <iostream>  // std::cout
#include <iterator>  // std::input_iterator_tag
#include <memory>    // std::shared_ptr
#include <queue>     // std::queue
#include <utility>   // std::pair
#include <vector>    // std::vector

namespace symbolic {

template <typename NodeT>
class BreadthFirstSearch {
 public:
  class iterator;

  BreadthFirstSearch(
      const NodeT& root, size_t max_depth, bool verbose = false,
      std::chrono::microseconds us_timeout = std::chrono::microseconds(0))
      : max_depth_(max_depth),
        verbose_(verbose),
        root_(root),
        timeout_(us_timeout) {}

  iterator begin() const {
    iterator it(this);
    return ++it;
  }
  iterator end() const { return iterator(); }

 private:
  const size_t max_depth_;
  const bool verbose_;
  const std::chrono::microseconds timeout_;

  const NodeT& root_;
};

template <typename NodeT>
class BreadthFirstSearch<NodeT>::iterator {
 public:
  using iterator_category = std::input_iterator_tag;
  using value_type = std::vector<NodeT>;
  using difference_type = ptrdiff_t;
  using pointer = const value_type*;
  using reference = const value_type&;

  iterator() = default;
  explicit iterator(const BreadthFirstSearch<NodeT>* bfs)
      : bfs_(bfs),
        queue_({{bfs_->root_, std::make_shared<std::vector<NodeT>>()}}) {}

  iterator& operator++();

  bool operator==(const iterator& other) const {
    return IsFinished() && other.IsFinished();
  }

  bool operator!=(const iterator& other) const { return !(*this == other); }

  reference operator*() const { return *ancestors_; }

 private:
  const BreadthFirstSearch<NodeT>* bfs_ = nullptr;
  bool IsFinished() const { return queue_.empty() && !ancestors_; }

  std::queue<std::pair<NodeT, std::shared_ptr<std::vector<NodeT>>>> queue_;
  std::shared_ptr<std::vector<NodeT>> ancestors_;
};

template <typename NodeT>
typename BreadthFirstSearch<NodeT>::iterator&
BreadthFirstSearch<NodeT>::iterator::operator++() {
  const auto t_start = std::chrono::high_resolution_clock::now();
  size_t depth = 0;
  while (!queue_.empty()) {
    // Abort on timeout.
    if (bfs_->timeout_.count() > 0 &&
        std::chrono::high_resolution_clock::now() - t_start > bfs_->timeout_) {
      std::queue<std::pair<NodeT, std::shared_ptr<std::vector<NodeT>>>> empty;
      std::swap(queue_, empty);
      break;
    }

    std::pair<NodeT, std::shared_ptr<std::vector<NodeT>>>& front =
        queue_.front();

    // Take ancestors list and append current node
    ancestors_ = std::make_shared<std::vector<NodeT>>(*front.second);
    ancestors_->push_back(std::move(front.first));
    queue_.pop();
    const NodeT& node = ancestors_->back();

    // Print search depth
    if (bfs_->verbose_ && ancestors_->size() > depth) {
      depth = ancestors_->size();
      std::cout << "BFS depth: " << depth - 1 << std::endl;
    }

    // Return if node evaluates to true
    if (node) {
      if (bfs_->verbose_) {
        std::cout << "Goal state reached: " << node << std::endl;
      }
      return *this;
    }

    // Skip children if max depth has been reached
    if (ancestors_->size() > bfs_->max_depth_) continue;

    // Add node's children to queue
    if (bfs_->verbose_) {
      for (const NodeT& node : *ancestors_) {
        std::cout << node << std::endl;
      }
      std::cout << "====================" << std::endl;
    }
    for (const NodeT& child : node) {
      // Print node
      if (bfs_->verbose_) {
        std::cout << child << std::endl << std::endl;
      }

      queue_.emplace(child, ancestors_);
    }
  }
  ancestors_.reset();
  return *this;
}

}  // namespace symbolic

#endif  // SYMBOLIC_PLANNING_BREADTH_FIRST_SEARCH_H_
