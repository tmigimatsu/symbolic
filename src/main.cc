/**
 * main.cc
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 21, 2018
 * Authors: Toki Migimatsu
 */

#include <iostream>  // std::cout
#include <set>       // std::set
#include <string>    // std::stoi
#include <vector>    // std::vector

#include "symbolic/planning/a_star.h"
#include "symbolic/planning/breadth_first_search.h"
#include "symbolic/planning/depth_first_search.h"
#include "symbolic/pddl.h"
#include "symbolic/planning/planner.h"

namespace {

struct Args {
  std::string filename_domain;
  std::string filename_problem;
  size_t depth = 0;
};

Args ParseArgs(int argc, char *argv[]) {
  Args parsed_args;
  int i;
  std::string arg;
  for (i = 1; i < argc; i++) {
    arg = argv[i];
    if (parsed_args.filename_domain.empty()) {
      parsed_args.filename_domain = argv[i];
    } else if (parsed_args.filename_problem.empty()) {
      parsed_args.filename_problem = argv[i];
    } else if (i == 3) {
      parsed_args.depth = std::stoi(argv[i]);
    } else {
      break;
    }
  }

  if (parsed_args.filename_domain.empty() || parsed_args.filename_problem.empty()) {
    throw std::invalid_argument("ParseArgs(): PDDL domain and problem files required.");
  }
  if (i != argc) throw std::invalid_argument("ParseArgs(): Invalid '" + arg + "' argument.");
  return parsed_args;
}

}  // namespace

int main(int argc, char* argv[]) {
  Args args = ParseArgs(argc, argv);

  const symbolic::Pddl pddl(args.filename_domain, args.filename_problem);
  pddl.IsValid(true);

  std::cout << pddl << std::endl;

  symbolic::Planner planner(pddl);

  symbolic::BreadthFirstSearch<symbolic::Planner::Node> bfs(planner.root(), args.depth);
  for (const std::vector<symbolic::Planner::Node>& plan : bfs) {
    for (const symbolic::Planner::Node& node : plan) {
      std::cout << node << std::endl;
    }
    std::cout << std::endl;
  }

  // symbolic::DepthFirstSearch<symbolic::Planner::Node> dfs(planner.root(), 5);
  // for (const std::vector<symbolic::Planner::Node>& plan : dfs) {
  //   for (const symbolic::Planner::Node& node : plan) {
  //     std::cout << node << std::endl;
  //   }
  //   std::cout << std::endl;
  // }

  // auto Heuristic = [](const symbolic::SearchNode<symbolic::Planner::Node>& left,
  //                     const symbolic::SearchNode<symbolic::Planner::Node>& right) -> bool {
  //   return left.ancestors.size() > right.ancestors.size();
  // };
  // symbolic::AStar<symbolic::Planner::Node, decltype(Heuristic)> astar(Heuristic, planner.root(), 5);
  // for (const std::vector<symbolic::Planner::Node>& plan : astar) {
  //   // for (const symbolic::Planner::Node& node : plan) {
  //   //   std::cout << node << std::endl;
  //   // }
  //   // std::cout << std::endl;
  // }


  // std::vector<std::vector<int>> options = {{11,12},{21,22,23},{31,32}};
  // symbolic::CombinationGenerator<std::vector<int>> gen({{11,12},{21,22,23},{31,32}});
  // for (auto it = gen.begin(); it != gen.end(); ++it) {
  //   const auto& values = *it;
  //   for (int v : values) {
  //     std::cout << v << " ";
  //   }
  //   std::cout << std::endl;
  // }
  // std::cout << std::endl;
  // for (auto it = gen.crbegin(); it != gen.crend(); ++it) {
  //   for (int v : *it) {
  //     std::cout << v << " ";
  //   }
  //   std::cout << std::endl;
  // }
}
