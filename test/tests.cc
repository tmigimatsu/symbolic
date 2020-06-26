/**
 * tests.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: April 1, 2020
 * Authors: Toki Migimatsu
 */

#define CATCH_CONFIG_MAIN
#include <algorithm>  // std::transform
#include <array>      // std::array
#include <catch2/catch.hpp>
#include <chrono>    // std::chrono
#include <iostream>  // std::cout
#include <iterator>  // std::back_inserter
#include <vector>    // std::vector

#include "symbolic/normal_form.h"
#include "symbolic/pddl.h"
#include "symbolic/planning/breadth_first_search.h"
#include "symbolic/planning/planner.h"
#include "symbolic/utils/combination_generator.h"

TEST_CASE("combination_generator", "[CombinationGenerator]") {
  std::vector<std::string> a = {"a", "b", "c", "d"};
  std::vector<std::string> b = {"A", "B", "C", "D", "E"};
  std::vector<std::string> c = {"1", "2", "3"};
  const std::vector<std::string> a_const = {"a", "b", "c", "d"};
  const std::vector<std::string> b_const = {"A", "B", "C", "D", "E"};
  const std::vector<std::string> c_const = {"1", "2", "3"};

  symbolic::CombinationGenerator<std::vector<std::string>> gen_empty;
  symbolic::CombinationGenerator<std::vector<std::string>> gen({&a, &b, &c});
  symbolic::CombinationGenerator<const std::vector<std::string>> gen_const(
      {&a_const, &b_const, &c_const});

  SECTION("size") {
    // Check empty size
    REQUIRE(gen_empty.size() == 0);
    REQUIRE(gen_empty.empty() == true);

    // Check size
    REQUIRE(gen.size() == a.size() * b.size() * c.size());
    REQUIRE(gen.empty() == false);
  }

  SECTION("mutability") {
    std::array<int, 6> a = {0, 1, 2, 3, 4, 5};
    std::array<int*, 3> a_ref = {&a[0], &a[1], &a[2]};
    std::array<int*, 3> b_ref = {&a[3], &a[4], &a[5]};
    symbolic::CombinationGenerator<std::array<int*, 3>> gen_mut({&a_ref, &b_ref});
    for (std::vector<int*> ab : gen_mut) {
      int& a = *ab[0];
      int& b = *ab[1];
      b++;
    }
    REQUIRE(a == std::array<int, 6>{0, 1, 2, 6, 7, 8});
  }

  SECTION("forward iterator") {
    // Check iterator equality
    auto it = gen.begin();
    REQUIRE(it == gen.begin());
    REQUIRE(it != gen.end());

    // Check iterator dereference
    REQUIRE(it->size() == 3);
    REQUIRE(*it == std::vector<std::string>{"a", "A", "1"});

    // Check iterator increment
    REQUIRE(*it++ == std::vector<std::string>{"a", "A", "1"});
    REQUIRE(*it == std::vector<std::string>{"a", "A", "2"});
    REQUIRE(*++it == std::vector<std::string>{"a", "A", "3"});
    REQUIRE(*++it == std::vector<std::string>{"a", "B", "1"});

    // Check end
    it = gen.begin();
    for (size_t i = 0; i < gen.size() - 1; i++) {
      ++it;
    }
    REQUIRE(*it == std::vector<std::string>{"d", "E", "3"});
    REQUIRE(++it == gen.end());

    // Check empty
    REQUIRE(gen_empty.begin() == gen_empty.end());

    // Check const
    REQUIRE(*gen_const.begin() == std::vector<std::string>{"a", "A", "1"});
    REQUIRE(*++gen_const.begin() == std::vector<std::string>{"a", "A", "2"});
  }

  SECTION("bidirectional iterator") {
    // Check decrement
    auto it = gen.end();
    REQUIRE(it-- == gen.end());
    REQUIRE(*it == std::vector<std::string>{"d", "E", "3"});
    REQUIRE(*--it == std::vector<std::string>{"d", "E", "2"});
    REQUIRE(*--it == std::vector<std::string>{"d", "E", "1"});
    REQUIRE(*--it == std::vector<std::string>{"d", "D", "3"});

    // Check beginning
    it = --gen.end();
    for (size_t i = 0; i < gen.size() - 1; i++) {
      --it;
    }
    REQUIRE(it == gen.begin());

    // Check const
    REQUIRE(*--gen_const.end() == std::vector<std::string>{"d", "E", "3"});
  }

  SECTION("random access iterator") {
    // Check inequalities
    REQUIRE(gen.begin() < ++gen.begin());
    REQUIRE(gen.end() > gen.begin());
    REQUIRE(gen.begin() <= gen.begin());
    REQUIRE(gen.end() >= gen.begin());

    // Check addition
    auto it = gen.begin();
    it += 3;
    REQUIRE(*it == std::vector<std::string>{"a", "B", "1"});
    REQUIRE(gen.begin() + gen.size() == gen.end());

    // Check subtraction
    it = gen.end();
    it -= 4;
    REQUIRE(*it == std::vector<std::string>{"d", "D", "3"});
    REQUIRE(it == gen.end() - 4);
    REQUIRE(gen.end() - gen.begin() == gen.size());

    REQUIRE(gen.at(gen.size() - 1) == std::vector<std::string>{"d", "E", "3"});
    REQUIRE(gen[1] == std::vector<std::string>{"a", "A", "2"});
  }

  SECTION("reverse iterator") {
    REQUIRE(*gen.rbegin() == std::vector<std::string>{"d", "E", "3"});
    REQUIRE(*++gen.rbegin() == std::vector<std::string>{"d", "E", "2"});
    REQUIRE(*--gen.rend() == std::vector<std::string>{"a", "A", "1"});
    REQUIRE(*gen_const.rbegin() == std::vector<std::string>{"d", "E", "3"});
    REQUIRE(*++gen_const.rbegin() == std::vector<std::string>{"d", "E", "2"});
    REQUIRE(*--gen_const.rend() == std::vector<std::string>{"a", "A", "1"});

    // Check mutability
    std::array<int, 6> a = {0, 1, 2, 3, 4, 5};
    std::array<int*, 3> a_ref = {&a[0], &a[1], &a[2]};
    std::array<int*, 3> b_ref = {&a[3], &a[4], &a[5]};
    symbolic::CombinationGenerator<std::array<int*, 3>> gen_mut({&a_ref, &b_ref});
    for (auto rit = gen_mut.rbegin(); rit != gen_mut.rend(); ++rit) {
      std::vector<int*> ab = *rit;
      int& a = *ab[0];
      int& b = *ab[1];
      b++;
    }
    REQUIRE(a == std::array<int, 6>{0, 1, 2, 6, 7, 8});
  }
};

TEST_CASE("pddl", "[Pddl]") {
  const symbolic::Pddl pddl("../resources/domain.pddl",
                            "../resources/problem.pddl");
  REQUIRE(pddl.IsValid());

  const symbolic::State& state = pddl.initial_state();
  const std::set<std::string> str_state = symbolic::Stringify(state);

  const symbolic::Action action(pddl, "pick");
  const symbolic::Object hook(pddl, "hook");
  const symbolic::Object box(pddl, "box");
  const std::string str_action = "pick(hook)";

  symbolic::State next_state = state;
  next_state.erase(symbolic::Proposition(pddl, "on(hook, table)"));
  next_state.emplace(pddl, "inhand(hook)");
  std::set<std::string> str_next_state = symbolic::Stringify(next_state);

  SECTION("IsValidAction") {
    REQUIRE(action.IsValid(state, {hook}) == true);
    REQUIRE(action.IsValid(state, {box}) == false);

    REQUIRE(pddl.IsValidAction(str_state, str_action) == true);
    REQUIRE(pddl.IsValidAction(str_state, "pick(box)") == false);
  }

  SECTION("NextState") {
    REQUIRE(action.Apply(state, {hook}) == next_state);
    REQUIRE(action.Apply(state, {hook}) != state);

    REQUIRE(pddl.NextState(str_state, str_action) == str_next_state);
    REQUIRE(pddl.NextState(str_state, str_action) != str_state);
  }

  SECTION("IsValidTuple") {
    REQUIRE(pddl.IsValidTuple(str_state, str_action, str_next_state) == true);
    REQUIRE(pddl.IsValidTuple(str_state, str_action, str_state) == false);
  }

  SECTION("IsGoalSatisfied") {
    std::set<std::string> str_goal_state = str_state;
    str_goal_state.emplace("on(box, shelf)");

    REQUIRE(pddl.IsGoalSatisfied(str_goal_state) == true);
    REQUIRE(pddl.IsGoalSatisfied(str_state) == false);
  }

  SECTION("IsValidPlan") {
    const std::vector<std::string> action_skeleton = {
        "pick(hook)", "push(hook, box, table)", "place(hook, table)",
        "pick(box)", "place(box, shelf)"};
    REQUIRE(pddl.IsValidPlan(action_skeleton) == true);
    REQUIRE(pddl.IsValidPlan({"pick(hook)"}) == false);
  }

  SECTION("ListValidArguments") {
    const std::vector<std::vector<symbolic::Object>> arguments = {{hook}};
    const std::vector<std::vector<std::string>> str_arguments = {{"hook"}};
    REQUIRE(pddl.ListValidArguments(state, action) == arguments);
    REQUIRE(pddl.ListValidArguments(str_state, "pick") == str_arguments);
  }

  SECTION("ListValidActions") {
    const std::vector<std::string> actions = {{"pick(hook)"}};
    REQUIRE(pddl.ListValidActions(state) == actions);
    REQUIRE(pddl.ListValidActions(str_state) == actions);
  }

  // symbolic::Planner planner(pddl);
  // symbolic::BreadthFirstSearch<symbolic::Planner::Node> bfs(planner.root(),
  // 4); for (const std::vector<symbolic::Planner::Node>& plan : bfs) {
  //   for (const symbolic::Planner::Node& node : plan) {
  //     std::cout << node << std::endl;
  //   }
  //   std::cout << std::endl;
  // }
}

TEST_CASE("DisjunctiveFormula", "[DisjunctiveFormula]") {
  const symbolic::Pddl pddl("../resources/domain.pddl",
                            "../resources/problem.pddl");
  const symbolic::Pddl pddl2("../resources/gridworld_domain.pddl",
                             "../resources/gridworld_problem.pddl");

  const symbolic::Action action(pddl, "pick");
  const symbolic::Object hook(pddl, "hook");

  symbolic::DisjunctiveFormula precond(pddl, action.preconditions(),
                                       action.parameters(), {hook});
  REQUIRE(precond == symbolic::DisjunctiveFormula(
                         {{{symbolic::Proposition(pddl, "inworkspace(hook)")},
                           {symbolic::Proposition(pddl, "inhand(box)"),
                            symbolic::Proposition(pddl, "inhand(hook)")}}}));

  symbolic::DisjunctiveFormula neg_precond =
      symbolic::Negate(pddl, std::move(precond)).value();
  REQUIRE(neg_precond ==
          symbolic::DisjunctiveFormula(
              {{{}, {symbolic::Proposition(pddl, "inworkspace(hook)")}},
               {{symbolic::Proposition(pddl, "inhand(box)")}, {}},
               {{symbolic::Proposition(pddl, "inhand(hook)")}, {}}}));

  symbolic::DisjunctiveFormula postcond(pddl, action.postconditions(),
                                        action.parameters(), {hook});
  REQUIRE(postcond ==
          symbolic::DisjunctiveFormula(
              {{{symbolic::Proposition(pddl, "inhand(hook)")},
                {symbolic::Proposition(pddl, "on(hook, box)"),
                 symbolic::Proposition(pddl, "on(hook, hook)"),
                 symbolic::Proposition(pddl, "on(hook, shelf)"),
                 symbolic::Proposition(pddl, "on(hook, table)")}}}));

  // symbolic::State state = pddl2.initial_state();
  // REQUIRE(pddl2.IsValidAction(state, "goto(door_key)"));
  // symbolic::State next_state = pddl2.NextState(state, "goto(door_key)");
  // std::cout << state << std::endl;
  // std::cout << next_state << std::endl;

  const auto t_start = std::chrono::high_resolution_clock::now();
  const auto cond = symbolic::DisjunctiveFormula::NormalizeConditions(
      pddl2, "goto(door_key)");
  const auto t_end = std::chrono::high_resolution_clock::now();

  // for (const symbolic::Axiom& axiom : pddl2.axioms()) {
  //   std::cout << "Axiom: " << axiom << std::endl;
  //   symbolic::DisjunctiveFormula context(pddl, axiom.preconditions(),
  //   axiom.parameters(), { hook, hook }); symbolic::DisjunctiveFormula
  //   implies(pddl, axiom.postconditions(), axiom.parameters(), { hook, hook
  //   }); std::cout << "  Context: " << context << std::endl; std::cout << "
  //   Implies: " << implies << std::endl; std::cout << "  Evaluation: " <<
  //   axiom.IsConsistent(pddl2.initial_state()) << std::endl;
  // }

  std::cout << cond->first << std::endl << cond->second << std::endl;
  std::cout << cond->first.conjunctions.size() << " "
            << cond->second.conjunctions.size() << std::endl;
  std::cout << std::chrono::duration<double>(t_end - t_start).count()
            << std::endl;
}
