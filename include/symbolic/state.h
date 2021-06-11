/**
 * state.h
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: April 23, 2020
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_STATE_H_
#define SYMBOLIC_STATE_H_

#define SYMBOLIC_STATE_USE_SET

#include <Eigen/Eigen>
#include <exception>      // std::exception
#include <functional>     // std::hash
#include <optional>       // std::optional
#include <ostream>        // std::ostream
#include <unordered_map>  // std::unordered_map
#include <unordered_set>  // std::unordered_set
#include <utility>        // std::pair

#ifndef SYMBOLIC_STATE_USE_SET
#include "symbolic/utils/unique_vector.h"
#else  // SYMBOLIC_STATE_USE_SET
#include "symbolic/utils/hash_set.h"
#endif  // SYMBOLIC_STATE_USE_SET

#include "symbolic/proposition.h"

namespace symbolic {

class Predicate;

#ifndef SYMBOLIC_STATE_USE_SET
class State : private UniqueVector<Proposition> {
  using Base = UniqueVector<Proposition>;
#else   // SYMBOLIC_STATE_USE_SET
class State : private HashSet<Proposition> {
  using Base = HashSet<Proposition>;
#endif  // SYMBOLIC_STATE_USE_SET

 public:
  using iterator = Base::iterator;
  using const_iterator = Base::const_iterator;

  State() = default;
  State(std::initializer_list<Proposition> l) : Base(l){};

  State(const Pddl& pddl, const std::unordered_set<std::string>& str_state);

  /**
   * Returns whether the state contains the given proposition.
   */
  bool contains(const PropositionBase& prop) const {
    return Base::contains(prop);
  }

  /**
   * Inserts a proposition into the state, and returns whether or not the state
   * has changed.
   */
  bool insert(const PropositionBase& prop) { return Base::insert(prop); }
  bool insert(Proposition&& prop) { return Base::insert(std::move(prop)); }

  template <class InputIt>
  bool insert(InputIt first, InputIt last);

  /**
   * Emplaces a proposition into the state, and returns whether or not the state
   * has changed.
   */
  template <class... Args>
  bool emplace(Args&&... args) {
    return insert(Proposition(args...));
  }

  /**
   * Removes a proposition from the state, and returns whether or not the state
   * has changed.
   */
  bool erase(const PropositionBase& prop) { return Base::erase(prop); }

  const_iterator begin() const { return Base::begin(); }
  const_iterator end() const { return Base::end(); };

  iterator begin() { return Base::begin(); }
  iterator end() { return Base::end(); }

  bool empty() const { return Base::empty(); }
  size_t size() const { return Base::size(); }

#ifndef SYMBOLIC_STATE_USE_SET
  void reserve(size_t size) { static_cast<Base&>(*this).reserve(size); }
#else   // SYMBOLIC_STATE_USE_SET
  void reserve(size_t size) {}
#endif  // SYMBOLIC_STATE_USE_SET

  std::unordered_set<std::string> Stringify() const;

  friend bool operator==(const State& lhs, const State& rhs) {
    return static_cast<const Base&>(lhs) == static_cast<const Base&>(rhs);
  }
  friend bool operator!=(const State& lhs, const State& rhs) {
    return !(lhs == rhs);
  }

  friend bool operator<(const State& lhs, const State& rhs) {
    return static_cast<const Base&>(lhs) < static_cast<const Base&>(rhs);
  }

  friend std::ostream& operator<<(std::ostream& os, const State& state);
};

template <class InputIt>
bool State::insert(InputIt first, InputIt last) {
  // TODO(tmigimatsu): More efficient way of inserting
  bool is_changed = false;
  for (InputIt it = first; it != last; ++it) {
    is_changed |= insert(*it);
  }
  return is_changed;
}

class PartialState {
 public:
  class UnknownEvaluation : public std::exception {
   public:
    explicit UnknownEvaluation(const PropositionBase& prop)
        : prop_(prop), str_prop_(prop.to_string()) {}

    const char* what() const noexcept override { return str_prop_.c_str(); }

    const Proposition& proposition() const { return prop_; }

   private:
    const Proposition prop_;
    const std::string str_prop_;
  };

  PartialState() = default;

  PartialState(const State& pos, const State& neg) : pos_(pos), neg_(neg) {}

  PartialState(State&& pos, State&& neg)
      : pos_(std::move(pos)), neg_(std::move(neg)) {}

  PartialState(const Pddl& pddl, const std::unordered_set<std::string>& str_pos,
               const std::unordered_set<std::string>& str_neg)
      : pos_(pddl, str_pos), neg_(pddl, str_neg) {}

  const State& pos() const { return pos_; }
  State& pos() { return pos_; }

  const State& neg() const { return neg_; }
  State& neg() { return neg_; }

  bool empty() const { return pos_.empty() && neg_.empty(); }
  size_t size() const { return pos_.size() + neg_.size(); }

  bool contains(const PropositionBase& prop) const;
  bool does_not_contain(const PropositionBase& prop) const;

  /**
   * Inserts the proposition into the positive state.
   *
   * If the proposition is negated, returns 2. If the proposition is simply
   * inserted, returns 1. If the proposition already exists, returns 0.
   */
  int insert(const PropositionBase& prop);
  int insert(Proposition&& prop);

  template <class... Args>
  int emplace(Args&&... args) {
    return insert(Proposition(args...));
  }

  /**
   * Inserts the proposition into the negative state.
   *
   * If the proposition is negated, returns 2. If the proposition is simply
   * inserted, returns 1. If the proposition already exists, returns 0.
   */
  int erase(const PropositionBase& prop);
  int erase(Proposition&& prop);

  /**
   * Ensure positive and negative proposition sets don't overlap.
   */
  bool IsConsistent() const;

  std::pair<std::unordered_set<std::string>, std::unordered_set<std::string>>
  Stringify() const;

  friend bool operator==(const PartialState& lhs, const PartialState& rhs) {
    return lhs.pos_ == rhs.pos_ && lhs.neg_ == rhs.neg_;
  }

  friend bool operator!=(const PartialState& lhs, const PartialState& rhs) {
    return lhs.pos_ != rhs.pos_ || lhs.neg_ != rhs.neg_;
  }

  friend bool operator<(const PartialState& lhs, const PartialState& rhs) {
    return std::tie(lhs.pos_, lhs.neg_) < std::tie(rhs.pos_, rhs.neg_);
  }

  friend std::ostream& operator<<(std::ostream& os, const PartialState& state);

 private:
  State pos_;
  State neg_;
};

/**
 * Database to convert between indexed and regular state.
 *
 * States are represented as Eigen boolean arrays.
 */
class StateIndex {
 public:
  class iterator;
  using IndexedState = Eigen::Array<bool, Eigen::Dynamic, 1>;

  /**
   * Construct the index from the given predicates.
   *
   * @param predicates Pddl predicates.
   * @param use_cache Cache proposition lookups.
   */
  explicit StateIndex(const std::vector<Predicate>& predicates,
                      bool use_cache = true);

  /**
   * Get a proposition from the index.
   *
   * @param idx_proposition Index of proposition.
   * @return Proposition at given index.
   */
  Proposition GetProposition(size_t idx_proposition) const;

  /**
   * Get the index of a proposition.
   *
   * @param prop Proposition.
   * @return Proposition index.
   */
  size_t GetPropositionIndex(const Proposition& prop) const;

  /**
   * Convert the indexed state to a full state.
   *
   * @param indexed_state Indexed state.
   * @return Full state.
   */
  State GetState(Eigen::Ref<const IndexedState> indexed_state) const;

  /**
   * Convert the state into an indexed state.
   *
   * @param state State.
   * @return Indexed state.
   */
  IndexedState GetIndexedState(const State& state) const;

  /**
   * Size of indexed state (total number of propositions).
   */
  size_t size() const { return idx_predicate_group_.back(); }

  // Iterators
  iterator begin() const { return iterator(this, 0); };
  iterator end() const { return iterator(this, size()); };

  const Pddl& pddl() const { return *pddl_; }

 private:
  const Pddl* pddl_ = nullptr;

  // Predicates vector stored for portability
  std::vector<Predicate> predicates_;

  // Sorted vector of predicate group indices in indexed state (beginning of
  // predicate group)
  std::vector<size_t> idx_predicate_group_;

  // Map from predicate to index in predicates vector
  std::unordered_map<std::string, size_t> idx_predicates_;

  // Cache
  mutable std::unordered_map<size_t, Proposition> cache_propositions_;
  mutable std::unordered_map<std::string, size_t> cache_idx_propositions_;

  bool use_cache_;

 public:
  class iterator {
   public:
    // Iterator traits
    using iterator_category = std::random_access_iterator_tag;
    using value_type = Proposition;
    using difference_type = ptrdiff_t;
    using pointer = const Proposition*;
    using reference = const Proposition&;

    // Constructor
    iterator(const StateIndex* state_index, int idx)
        : state_index_(state_index), idx_(idx) {}

    // Forward iterator
    iterator& operator++() { return operator+=(1); }

    iterator operator++(int) {
      iterator it = *this;
      operator++();
      return it;
    }

    reference operator*() const {
      prop_ = state_index_->GetProposition(idx_);
      return prop_;
    };

    pointer operator->() const {
      prop_ = state_index_->GetProposition(idx_);
      return &prop_;
    }

    bool operator==(const iterator& rhs) const {
      return state_index_ == rhs.state_index_ && idx_ == rhs.idx_;
    }

    bool operator!=(const iterator& rhs) const { return !(*this == rhs); }

    // Bidirectional iterator
    iterator& operator--() { return operator+=(-1); }

    iterator operator--(int) {
      iterator it = *this;
      operator--();
      return it;
    }

    // Random access iterator
    bool operator<(const iterator& rhs) const {
      return state_index_ == rhs.state_index_ && idx_ < rhs.idx_;
    }

    bool operator>(const iterator& rhs) const { return rhs < *this; };

    bool operator<=(const iterator& rhs) const {
      return state_index_ == rhs.state_index_ && idx_ <= rhs.idx_;
    }

    bool operator>=(const iterator& rhs) const { return rhs <= *this; };

    iterator& operator+=(difference_type n) {
      idx_ += n;
      return *this;
    }

    iterator operator+(difference_type n) const {
      iterator temp = *this;
      return temp += n;
    }

    friend iterator operator+(difference_type n, const iterator& it) {
      return it + n;
    }

    iterator& operator-=(difference_type n) { return operator+=(-n); }

    iterator operator-(difference_type n) const {
      iterator temp = *this;
      return temp -= n;
    }

    difference_type operator-(const iterator& rhs) const {
      return idx_ - rhs.idx_;
    }

    value_type operator[](difference_type n) const { return *(operator+(n)); }

   private:
    const StateIndex* state_index_ = nullptr;
    int idx_ = 0;
    mutable Proposition prop_;
  };
};

}  // namespace symbolic

namespace std {

template <>
struct hash<symbolic::State> {
  size_t operator()(const symbolic::State& state) const noexcept;
};

}  // namespace std

#endif  // SYMBOLIC_STATE_H_
