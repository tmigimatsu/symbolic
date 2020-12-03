/**
 * combination_generator.h
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_UTILS_COMBINATION_GENERATOR_H_
#define SYMBOLIC_UTILS_COMBINATION_GENERATOR_H_

#include <algorithm>    // std::find
#include <cassert>      // assert
#include <cstddef>      // ptrdiff_t
#include <exception>    // std::invalid_argument, std::out_of_range
#include <iterator>     // std::random_access_iterator_tag, std::iterator_traits
#include <sstream>      // std::stringstream
#include <string>       // std::to_string
#include <type_traits>  // std::conditional_t, std::is_const
#include <vector>       // std::vector

namespace symbolic {

/**
 * Class for generating all combinations of a collection of sequences.
 *
 * This class contains no mutable member variables. All the state is held inside
 * the iterator, meaning multiple parallel instances can use the same generator
 * simultaneously.
 *
 * Assumes that each option has at least one element.
 */
template <typename ContainerT>
class CombinationGenerator {
 private:
  /**
   * Iterates over the combinations, incrementing elements from right to left.
   *
   * Produces combinations as a vector of elements, stored inside the iterator.
   */
  template <bool Const>
  class Iterator;

  /**
   * Iterates over the combinations in reverse order, incrementing elements from
   * left to right.
   */
  template <typename IteratorT>
  class ReverseIterator;

 public:
  // Iterator types
  using iterator = typename std::conditional_t<std::is_const<ContainerT>::value,
                                               Iterator<true>, Iterator<false>>;
  using const_iterator = Iterator<true>;

  using reverse_iterator = ReverseIterator<iterator>;
  using const_reverse_iterator = ReverseIterator<const_iterator>;

  // Constructors
  CombinationGenerator() = default;
  virtual ~CombinationGenerator() = default;

  explicit CombinationGenerator(const std::vector<ContainerT*>& options)
      : options_(options),
        size_groups_(ComputeGroupSizes(options)),
        size_(ComputeSize(options, size_groups_)) {
    for (size_t i = 0; i < options.size(); i++) {
      if (options[i]->begin() == options[i]->end()) {
        throw std::invalid_argument(
            "CombinationGenerator(): Empty option at position " +
            std::to_string(i) + ".");
      }
    }
  }

  // Iterators
  iterator begin() { return iterator(this, 0); };
  iterator end() { return iterator(this, size()); };
  const_iterator begin() const { return const_iterator(this, 0); };
  const_iterator end() const { return const_iterator(this, size()); };
  const_iterator cbegin() const { return const_iterator(this, 0); };
  const_iterator cend() const { return const_iterator(this, size()); };

  // Reverse iterators
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator crend() const {
    return const_reverse_iterator(begin());
  }

  /**
   * Number of total combinations.
   */
  size_t size() const { return size_; }

  /**
   * Whether this combination generator is empty.
   */
  bool empty() const { return size_ == 0; }

  /**
   * Access specified element with wrapping and bounds checking.
   */
  typename iterator::value_type at(int i) const {
    // Wrap around negative indices
    if (i < 0) i += size();

    // Check for bounds
    if (i >= size() || i < 0) {
      throw std::out_of_range("ParameterGenerator::at(" + std::to_string(i) +
                              "): index beyond bounds" +
                              std::to_string(size()) + ".");
    }
    return operator[](i);
  }

  /**
   * Access specified element without bounds checking.
   */
  typename iterator::value_type operator[](int i) const {
    return *(begin() + i);
  }

  /**
   * Get index of given combination.
   */
  int find(const typename iterator::value_type& combination) const {
    assert(combination.size() == options_.size());
    size_t idx = 0;

    // Iterate over digit slots
    for (size_t i = 0; i < combination.size(); i++) {
      const typename iterator::ValueT& element = combination[i];
      const ContainerT& option = *options_.at(i);

      // Find element in current option
      const auto it_element = std::find(option.begin(), option.end(), element);
      if (it_element == option.end()) {
        std::stringstream ss;
        ss << "CombinationGenerator::find(): No element " << element
           << " in slot " << i << ".";
        throw std::out_of_range(ss.str());
      }

      // Get index of element
      const size_t idx_element = it_element - option.begin();
      idx += size_groups_[i] * idx_element;
    }

    return idx;
  }

 private:
  /**
   * Compute the size of each digit group for converting between flat indices
   * and subarray indices.
   *
   * For example, if we have 3 options of size (3, 4, 5), then the group sizes
   * would be (4 * 5, 5, 1).
   */
  static std::vector<size_t> ComputeGroupSizes(
      const std::vector<ContainerT*>& options) {
    std::vector<size_t> size_groups(options.size());

    // Return if options is empty
    if (options.empty()) return size_groups;

    // Last group has digit size 1
    size_groups.back() = 1;

    // Compute group sizes from right to left
    for (int i = options.size() - 2; i >= 0; i--) {
      const size_t num_next = options[i + 1]->size();
      size_groups[i] = num_next * size_groups[i + 1];
    }
    return size_groups;
  }

  /**
   * Compute the total number of combinations.
   *
   * Reuses the size_groups computation for efficiency.
   */
  static size_t ComputeSize(const std::vector<ContainerT*>& options,
                            const std::vector<size_t>& size_groups) {
    if (options.empty()) return 0;
    return options.front()->size() * size_groups.front();
  }

  std::vector<ContainerT*> options_;
  std::vector<size_t> size_groups_;
  size_t size_ = 0;
};

template <typename ContainerT>
template <bool Const>
class CombinationGenerator<ContainerT>::Iterator {
 public:
  using IteratorT =
      typename std::conditional_t<Const, typename ContainerT::const_iterator,
                                  typename ContainerT::iterator>;
  using ValueT = typename std::iterator_traits<IteratorT>::value_type;

  // Iterator traits
  using iterator_category = std::random_access_iterator_tag;
  using value_type = std::vector<ValueT>;
  using difference_type = ptrdiff_t;
  using pointer = const value_type*;
  using reference = const value_type&;

  // Constructor
  Iterator(const CombinationGenerator* gen, int idx) : gen_(gen), idx_(idx) {
    UpdateCombination();
  }

  // Forward iterator
  Iterator& operator++() { return operator+=(1); }

  Iterator operator++(int) {
    Iterator it = *this;
    operator++();
    return it;
  }

  reference operator*() const { return combination_; };

  pointer operator->() const { return &combination_; }

  bool operator==(const Iterator& rhs) const {
    return gen_ == rhs.gen_ && idx_ == rhs.idx_;
  }

  bool operator!=(const Iterator& rhs) const { return !(*this == rhs); }

  // Bidirectional iterator
  Iterator& operator--() { return operator+=(-1); }

  Iterator operator--(int) {
    Iterator it = *this;
    operator--();
    return it;
  }

  // Random access iterator
  bool operator<(const Iterator& rhs) const {
    return gen_ == rhs.gen_ && idx_ < rhs.idx_;
  }

  bool operator>(const Iterator& rhs) const { return rhs < *this; };

  bool operator<=(const Iterator& rhs) const {
    return gen_ == rhs.gen_ && idx_ <= rhs.idx_;
  }

  bool operator>=(const Iterator& rhs) const { return rhs <= *this; };

  Iterator& operator+=(difference_type n) {
    UpdateCombination(idx_ + n);
    return *this;
  }

  Iterator operator+(difference_type n) const {
    Iterator temp = *this;
    return temp += n;
  }

  friend Iterator operator+(difference_type n, const Iterator& it) {
    return it + n;
  }

  Iterator& operator-=(difference_type n) { return operator+=(-n); }

  Iterator operator-(difference_type n) const {
    Iterator temp = *this;
    return temp -= n;
  }

  difference_type operator-(const Iterator& rhs) const {
    return idx_ - rhs.idx_;
  }

  value_type operator[](difference_type n) const { return *(operator+(n)); }

 private:
  /**
   * Sets the combination from the current combination index.
   *
   * Does nothing if the index exceeds the number of combinations.
   */
  void UpdateCombination() {
    // Do nothing if the index exceeds the number of combinations
    if (idx_ >= gen_->size() || idx_ < 0) return;

    // Preallocate combination if it hasn't been already
    const size_t num_options = gen_->options_.size();
    if (combination_.empty()) combination_.resize(num_options);

    int remaining = idx_;
    for (size_t i = 0; i < num_options; i++) {
      // Compute the index for the current bin
      int idx_i = remaining / gen_->size_groups_[i];
      remaining -= idx_i * gen_->size_groups_[i];

      // Set the value
      combination_[i] = gen_->options_[i]->at(idx_i);
    }
    assert(remaining == 0);
  }

  /**
   * Updates the current index and sets the combination to the new index.
   *
   * Sets values only for elements where idx_ differs from idx_new.
   */
  void UpdateCombination(int idx_new) {
    // Do nothing if the index is unchanged
    if (idx_ == idx_new) return;

    // If index is outside the bounds, set it to the index and return
    if (idx_new >= gen_->size() || idx_new < 0) {
      idx_ = idx_new;
      return;
    }

    // Preallocate combination if it hasn't been already
    const size_t num_options = gen_->options_.size();
    if (combination_.empty()) combination_.resize(num_options);

    int remaining = idx_;
    int remaining_new = idx_new;
    for (size_t i = 0; i < num_options; i++) {
      // Compute the index for the current bin
      int idx_i = remaining / gen_->size_groups_[i];
      int idx_i_new = remaining_new / gen_->size_groups_[i];
      remaining -= idx_i * gen_->size_groups_[i];
      remaining_new -= idx_i_new * gen_->size_groups_[i];

      // Skip if the index is unchanged
      if (idx_i == idx_i_new) continue;

      // Set the value
      combination_[i] = gen_->options_[i]->at(idx_i_new);
    }
    idx_ = idx_new;
    assert(remaining == 0);
  }

  const CombinationGenerator* gen_ = nullptr;
  std::vector<ValueT> combination_;
  int idx_;

  friend class CombinationGenerator;
};

template <typename ContainerT>
template <typename IteratorT>
class CombinationGenerator<ContainerT>::ReverseIterator {
 public:
  // Iterator traits
  using iterator_category =
      typename std::iterator_traits<IteratorT>::iterator_category;
  using value_type = typename std::iterator_traits<IteratorT>::value_type;
  using difference_type =
      typename std::iterator_traits<IteratorT>::difference_type;
  using pointer = typename std::iterator_traits<IteratorT>::pointer;
  using reference = typename std::iterator_traits<IteratorT>::reference;

  explicit ReverseIterator(IteratorT&& it) : it_(std::move(--it)) {}

  // Forward iterator
  ReverseIterator& operator++() {
    --it_;
    return *this;
  }

  ReverseIterator operator++(int) {
    ReverseIterator it = *this;
    operator++();
    return it;
  }

  reference operator*() const { return *it_; }

  pointer operator->() const { return it_.operator->(); }

  bool operator==(const ReverseIterator& rhs) const { return it_ == rhs.it_; }

  bool operator!=(const ReverseIterator& rhs) const { return it_ != rhs.it_; }

  // Bidirectional iterator
  ReverseIterator& operator--() {
    ++it_;
    return *this;
  };

  ReverseIterator operator--(int) {
    ReverseIterator it = *this;
    operator--();
    return it;
  }

  // Random access iterator
  bool operator<(const ReverseIterator& rhs) const { return it_ > rhs.it_; }

  bool operator>(const ReverseIterator& rhs) const { return rhs < *this; };

  bool operator<=(const ReverseIterator& rhs) const { return it_ >= rhs.it_; }

  bool operator>=(const ReverseIterator& rhs) const { return rhs <= *this; };

  ReverseIterator& operator+=(difference_type n) {
    it_ -= n;
    return *this;
  }

  ReverseIterator operator+(difference_type n) const {
    ReverseIterator temp = *this;
    return temp += n;
  }

  friend ReverseIterator operator+(difference_type n,
                                   const ReverseIterator& it) {
    return it + n;
  }

  ReverseIterator& operator-=(difference_type n) { return operator+=(-n); }

  ReverseIterator operator-(difference_type n) const {
    Iterator temp = *this;
    return temp -= n;
  }

  difference_type operator-(const ReverseIterator& rhs) const {
    return rhs.it_ - it_;
  }

  value_type operator[](difference_type n) const { return *(operator+(n)); }

 private:
  IteratorT it_;
};

}  // namespace symbolic

#endif  // SYMBOLIC_UTILS_COMBINATION_GENERATOR_H_
