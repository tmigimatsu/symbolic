/**
 * hash_set.h
 *
 * Copyright 2021. All Rights Reserved.
 *
 * Created: April 15, 2021
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_UTILS_HASH_SET_H_
#define SYMBOLIC_UTILS_HASH_SET_H_

#include <algorithm>  // std::max, std::swap
#include <iostream>   // TODO: remove
#include <vector>     // std::vector

#include "symbolic/utils/unique_vector.h"

namespace symbolic {

constexpr int HASH_SET_INITIAL_SIZE = 1;

/**
 * Hash set implemented as a vector of unique sorted vectors.
 */
template <typename T>
class HashSet {
 public:
  class iterator;
  class const_iterator;
  // using iterator = const_iterator;

  HashSet() : buckets_(HASH_SET_INITIAL_SIZE){};

  HashSet(std::initializer_list<T> l) : buckets_(HASH_SET_INITIAL_SIZE) {
    for (const T& element : l) {
      insert(element);
    }
  }

  iterator begin() {
    iterator it(buckets_, 0, 0);
    it.FindNextElement();
    return it;
  }
  iterator end() { return iterator(buckets_, buckets_.size(), 0); }

  const_iterator begin() const {
    const_iterator it(buckets_, 0, 0);
    it.FindNextElement();
    return it;
  }
  const_iterator end() const {
    return const_iterator(buckets_, buckets_.size(), 0);
  }
  // iterator rend() const { return iterator(buckets_, -1, -1); }

  bool empty() const { return size() == 0; }
  size_t size() const { return size_; }

  size_t bucket_count() const { return buckets_.size(); }

  template <typename T_query>
  bool contains(const T_query& element) const {
    return GetBucket(element).contains(element);
  }

  template <typename T_query>
  bool insert(const T_query& element) {
    const bool inserted = GetBucket(element).insert(element);
    if (inserted) {
      size_++;
      if (size() > buckets_.size()) Rehash(UpperBound());
    }
    return inserted;
  }

  bool insert(T&& element) {
    const bool inserted = GetBucket(element).insert(std::move(element));
    if (inserted) {
      size_++;
      if (size() > buckets_.size()) Rehash(UpperBound());
    }
    return inserted;
  }

  template <typename T_query>
  bool erase(const T_query& element) {
    const bool erased = GetBucket(element).erase(element);
    if (erased) {
      size_--;
      if (size() <= LowerBound()) Rehash(LowerBound());
    }
    return erased;
  }

  friend bool operator==(const HashSet<T>& lhs, const HashSet<T>& rhs) {
    return lhs.buckets_ == rhs.buckets_;
  }
  friend bool operator!=(const HashSet<T>& lhs, const HashSet<T>& rhs) {
    return !(lhs == rhs);
  }

  friend bool operator<(const HashSet<T>& lhs, const HashSet<T>& rhs) {
    return lhs.buckets_ < rhs.buckets_;
  }

 private:
  size_t UpperBound() const { return 2 * buckets_.size() + 1; }
  size_t LowerBound() const {
    return std::max(HASH_SET_INITIAL_SIZE, (static_cast<int>(buckets_.size()) - 1) / 2);
  }

  template <typename T_query>
  UniqueVector<T>& GetBucket(const T_query& element) {
    const size_t idx_bucket = std::hash<T_query>{}(element) % buckets_.size();
    return buckets_[idx_bucket];
  }
  template <typename T_query>
  const UniqueVector<T>& GetBucket(const T_query& element) const {
    const size_t idx_bucket = std::hash<T_query>{}(element) % buckets_.size();
    return buckets_[idx_bucket];
  }

  void Rehash(size_t num_buckets) {
    if (num_buckets == buckets_.size()) return;

    // Create new buckets.
    std::vector<UniqueVector<T>> old_buckets(num_buckets);
    std::swap(buckets_, old_buckets);

    // Iterate over old buckets.
    for (UniqueVector<T>& bucket : old_buckets) {
      // Move elements from old bucket.
      for (T& element : bucket) {
        GetBucket(element).insert(std::move(element));
      }
    }
  }

  std::vector<UniqueVector<T>> buckets_;
  size_t size_ = 0;

 public:
  class const_iterator {
   public:
    // Iterator traits
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;

    // Constructor
    const_iterator(const std::vector<UniqueVector<T>>& buckets,
                   const int idx_bucket, const int idx_in_bucket)
        : buckets_(&buckets),
          idx_bucket_(idx_bucket),
          idx_in_bucket_(idx_in_bucket) {}

    // Forward iterator
    const_iterator& operator++() {
      idx_in_bucket_++;
      FindNextElement();
      return *this;
    }

    const_iterator operator++(int) {
      const_iterator it = *this;
      operator++();
      return it;
    }

    reference operator*() const {
      return (*buckets_)[idx_bucket_][idx_in_bucket_];
    }

    pointer operator->() const {
      return &(*buckets_)[idx_bucket_][idx_in_bucket_];
    }

    bool operator==(const const_iterator& rhs) const {
      return idx_bucket_ == rhs.idx_bucket_ &&
             idx_in_bucket_ == rhs.idx_in_bucket_;
    }

    bool operator!=(const const_iterator& rhs) const { return !(*this == rhs); }

    // Bidirectional iterator
    const_iterator& operator--() {
      idx_in_bucket_--;
      FindPreviousElement();
      return *this;
    }

    const_iterator operator--(int) {
      const_iterator it = *this;
      operator--();
      return it;
    }

   protected:
    friend HashSet<T>;

    void FindNextElement() {
      // Find next occupied bucket.
      if (idx_bucket_ >= buckets_->size()) {
        idx_in_bucket_ = 0;
        return;
      }
      const UniqueVector<T>* bucket = &(*buckets_)[idx_bucket_];
      while (idx_in_bucket_ >= bucket->size()) {
        idx_bucket_++;
        idx_in_bucket_ = 0;
        if (idx_bucket_ == buckets_->size()) return;
        bucket = &(*buckets_)[idx_bucket_];
      }
    }
    void FindPreviousElement() {
      // Find previous occupied bucket.
      while (idx_in_bucket_ < 0) {
        idx_bucket_--;
        if (idx_bucket_ < 0) return;
        const UniqueVector<T>& bucket = (*buckets_)[idx_bucket_];
        idx_in_bucket_ = bucket.size() - 1;
      }
    }

    const std::vector<UniqueVector<T>>* buckets_ = nullptr;
    int idx_bucket_ = 0;
    int idx_in_bucket_ = 0;
  };
  class iterator {
   public:
    // Iterator traits
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    // Constructor
    iterator(std::vector<UniqueVector<T>>& buckets, const int idx_bucket,
             const int idx_in_bucket)
        : buckets_(&buckets),
          idx_bucket_(idx_bucket),
          idx_in_bucket_(idx_in_bucket) {}

    // Forward iterator
    iterator& operator++() {
      idx_in_bucket_++;
      FindNextElement();
      return *this;
    }

    iterator operator++(int) {
      iterator it = *this;
      operator++();
      return it;
    }

    reference operator*() const {
      return (*buckets_)[idx_bucket_][idx_in_bucket_];
    }

    pointer operator->() const {
      return &(*buckets_)[idx_bucket_][idx_in_bucket_];
    }

    bool operator==(const iterator& rhs) const {
      return idx_bucket_ == rhs.idx_bucket_ &&
             idx_in_bucket_ == rhs.idx_in_bucket_;
    }

    bool operator!=(const iterator& rhs) const { return !(*this == rhs); }

    // Bidirectional iterator
    iterator& operator--() {
      idx_in_bucket_--;
      FindPreviousElement();
      return *this;
    }

    iterator operator--(int) {
      iterator it = *this;
      operator--();
      return it;
    }

   protected:
    friend HashSet<T>;

    void FindNextElement() {
      // Find next occupied bucket.
      if (idx_bucket_ >= buckets_->size()) {
        idx_in_bucket_ = 0;
        return;
      }
      const UniqueVector<T>* bucket = &(*buckets_)[idx_bucket_];
      while (idx_in_bucket_ >= bucket->size()) {
        idx_bucket_++;
        idx_in_bucket_ = 0;
        if (idx_bucket_ == buckets_->size()) return;
        bucket = &(*buckets_)[idx_bucket_];
      }
    }

    void FindPreviousElement() {
      // Find previous occupied bucket.
      while (idx_in_bucket_ < 0) {
        idx_bucket_--;
        if (idx_bucket_ < 0) return;
        const UniqueVector<T>& bucket = (*buckets_)[idx_bucket_];
        idx_in_bucket_ = bucket.size() - 1;
      }
    }

    std::vector<UniqueVector<T>>* buckets_ = nullptr;
    int idx_bucket_ = 0;
    int idx_in_bucket_ = 0;
  };
};

}  // namespace symbolic

#endif  // SYMBOLIC_UTILS_HASH_SET_H_
