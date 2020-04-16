/**
 * main.cc
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 21, 2018
 * Authors: Toki Migimatsu
 */

#include <chrono>    // std::chrono
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
  size_t depth = 5;
};

Args ParseArgs(int argc, char *argv[]) {
  Args parsed_args;
  try {
    if (argc < 3) {
      throw std::runtime_error("Incorrect number of arguments.");
    }
    parsed_args.filename_domain = argv[1];
    parsed_args.filename_problem = argv[2];
    if (argc > 3) parsed_args.depth = std::stoi(argv[3]);
  } catch (const std::runtime_error& e) {
    std::cout << "Usage:" << std::endl
              << "\t./pddl domain.pddl problem.pddl [search_depth (default 5)]" << std::endl;
    throw e;
  }
  return parsed_args;
}

}  // namespace

int main(int argc, char* argv[]) {
  Args args = ParseArgs(argc, argv);
  std::cout << "Domain: " << args.filename_domain << std::endl
            << "Problem: " << args.filename_problem << std::endl
            << "Depth: " << args.depth << std::endl << std::endl;

  const symbolic::Pddl pddl(args.filename_domain, args.filename_problem);
  pddl.IsValid(true);

  std::cout << pddl << std::endl;

  symbolic::Planner planner(pddl);

  std::cout << "Planning:" << std::endl;
  const auto t_start = std::chrono::high_resolution_clock::now();
  symbolic::BreadthFirstSearch<symbolic::Planner::Node> bfs(planner.root(), args.depth);
  for (const std::vector<symbolic::Planner::Node>& plan : bfs) {
    std::cout << std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - t_start).count() << "s" << std::endl;
    for (const symbolic::Planner::Node& node : plan) {
      std::cout << node << std::endl;
    }
    std::cout << std::endl;
  }

  std::cout << "Finished in " << std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - t_start).count() << "s" << std::endl;

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
