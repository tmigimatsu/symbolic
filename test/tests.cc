/**
 * tests.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: April 1, 2020
 * Authors: Toki Migimatsu
 */

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <algorithm>  // std::transform
#include <iostream>   // std::cout
#include <iterator>   // std::back_inserter

#include "symbolic/normal_form.h"
#include "symbolic/pddl.h"
#include "symbolic/planning/breadth_first_search.h"
#include "symbolic/planning/planner.h"

namespace {

std::set<std::string> Stringify(const std::set<symbolic::Proposition>& state) {
  std::set<std::string> str_state;
  std::transform(state.begin(), state.end(), std::inserter(str_state, str_state.begin()),
                 [](const symbolic::Proposition& prop) { return prop.to_string(); });
  return str_state;
}

}  // namespace

TEST_CASE("pddl", "[Pddl]") {
  const symbolic::Pddl pddl("../resources/domain.pddl", "../resources/problem.pddl");
  REQUIRE(pddl.IsValid());

  const std::set<symbolic::Proposition>& state = pddl.initial_state();
  const std::set<std::string> str_state = pddl.initial_state_str();

  const symbolic::Action action(pddl, "pick");
  const symbolic::Object hook(pddl, "hook");
  const symbolic::Object box(pddl, "box");
  const std::string str_action = "pick(hook)";

  std::set<symbolic::Proposition> next_state = state;
  next_state.erase(symbolic::Proposition(pddl, "on(hook, table)"));
  next_state.emplace(pddl, "inhand(hook)");
  std::set<std::string> str_next_state = Stringify(next_state);

  SECTION("IsValidAction") {
    REQUIRE(action.IsValid(state, { hook }) == true);
    REQUIRE(action.IsValid(state, { box }) == false);

    REQUIRE(pddl.IsValidAction(str_state, str_action) == true);
    REQUIRE(pddl.IsValidAction(str_state, "pick(box)") == false);
  }

  SECTION("NextState") {
    REQUIRE(action.Apply(state, { hook }) == next_state);
    REQUIRE(action.Apply(state, { hook }) != state);

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
      "pick(hook)",
      "push(hook, box, table)",
      "place(hook, table)",
      "pick(box)",
      "place(box, shelf)"
    };
    REQUIRE(pddl.IsValidPlan(action_skeleton) == true);
    REQUIRE(pddl.IsValidPlan({ "pick(hook)" }) == false);
  }

  SECTION("ListValidArguments") {
    const std::vector<std::vector<symbolic::Object>> arguments = {{ hook }};
    const std::vector<std::vector<std::string>> str_arguments = {{ "hook" }};
    REQUIRE(pddl.ListValidArguments(state, action) == arguments);
    REQUIRE(pddl.ListValidArguments(str_state, "pick") == str_arguments);
  }

  SECTION("ListValidActions") {
    const std::vector<std::string> actions = {{ "pick(hook)" }};
    REQUIRE(pddl.ListValidActions(state) == actions);
    REQUIRE(pddl.ListValidActions(str_state) == actions);
  }

  // symbolic::Planner planner(pddl);
  // symbolic::BreadthFirstSearch<symbolic::Planner::Node> bfs(planner.root(), 4);
  // for (const std::vector<symbolic::Planner::Node>& plan : bfs) {
  //   for (const symbolic::Planner::Node& node : plan) {
  //     std::cout << node << std::endl;
  //   }
  //   std::cout << std::endl;
  // }
}

TEST_CASE("DisjunctiveFormula", "[DisjunctiveFormula]") {
  const symbolic::Pddl pddl("../resources/domain.pddl", "../resources/problem.pddl");

  const symbolic::Action action(pddl, "pick");
  const symbolic::Object hook(pddl, "hook");

  const std::vector<symbolic::Proposition> pos = { symbolic::Proposition(pddl, "inworkspace(hook)") };
  const std::vector<symbolic::Proposition> neg = { symbolic::Proposition(pddl, "inhand(hook)"),
                                                   symbolic::Proposition(pddl, "inhand(box)") };
;
  symbolic::DisjunctiveFormula precond(pddl, action.preconditions(), action.parameters(), { hook });
  REQUIRE(precond == symbolic::DisjunctiveFormula({{ pos, neg }}));

  symbolic::DisjunctiveFormula neg_precond = symbolic::Negate(std::move(precond));
  REQUIRE(neg_precond == symbolic::DisjunctiveFormula({{{ symbolic::Proposition(pddl, "inhand(hook)") }, {}},
                                                       {{ symbolic::Proposition(pddl, "inhand(box)") }, {}},
                                                       {{}, { symbolic::Proposition(pddl, "inworkspace(hook)") }}}));

  symbolic::DisjunctiveFormula postcond(pddl, action.postconditions(), action.parameters(), { hook });
  REQUIRE(postcond == symbolic::DisjunctiveFormula({{{ symbolic::Proposition(pddl, "inhand(hook)") },
                                                     { symbolic::Proposition(pddl, "on(hook, table)"),
                                                       symbolic::Proposition(pddl, "on(hook, shelf)"),
                                                       symbolic::Proposition(pddl, "on(hook, hook)"),
                                                       symbolic::Proposition(pddl, "on(hook, box)") }}}));
}
