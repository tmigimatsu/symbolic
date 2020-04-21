/**
 * symbolic.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: March 7, 2020
 * Authors: Toki Migimatsu
 */

#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include "symbolic/normal_form.h"
#include "symbolic/pddl.h"
#include "symbolic/planning/breadth_first_search.h"
#include "symbolic/planning/planner.h"

namespace symbolic {

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(symbolic, m) {

  // Pddl
  py::class_<Pddl>(m, "Pddl")
      .def(py::init<const std::string&, const std::string&>(), "domain"_a, "problem"_a)
      .def_property_readonly("initial_state", &Pddl::initial_state_str)
      .def_property_readonly("actions", &Pddl::actions_str)
      .def("is_valid", [](const Pddl& pddl, bool verbose) { return pddl.IsValid(verbose); }, "verbose"_a = false)
      .def("next_state", (std::set<std::string> (Pddl::*)(const std::set<std::string>&, const std::string&) const) &Pddl::NextState)
      .def("is_valid_action", (bool (Pddl::*)(const std::set<std::string>&, const std::string&) const) &Pddl::IsValidAction)
      .def("is_valid_tuple", (bool (Pddl::*)(const std::set<std::string>&, const std::string&, const std::set<std::string>&) const) &Pddl::IsValidTuple)
      .def("is_goal_satisfied", (bool (Pddl::*)(const std::set<std::string>&) const) &Pddl::IsGoalSatisfied)
      .def("is_valid_plan", &Pddl::IsValidPlan)
      .def("list_valid_arguments", (std::vector<std::vector<std::string>> (Pddl::*)(const std::set<std::string>&, const std::string&) const) &Pddl::ListValidArguments)
      .def("list_valid_actions", (std::vector<std::string> (Pddl::*)(const std::set<std::string>&) const) &Pddl::ListValidActions);

  // Planner::Node
  py::class_<Planner::Node>(m, "PlannerNode")
      .def_property_readonly("action", &Planner::Node::action)
      .def_property_readonly("state", [](const Planner::Node& node) {
        const std::set<Proposition>& state = node.state();
        std::set<std::string> str_state;
        for (const Proposition& prop : state) {
          str_state.emplace(prop.to_string());
        }
        return str_state;
      })
      .def_property_readonly("depth", &Planner::Node::depth)
      .def("__repr__", [](const Planner::Node& node) {
        std::stringstream ss;
        ss << node;
        return ss.str();
      });

  // Planner
  py::class_<Planner>(m, "Planner")
      .def(py::init<const Pddl&>(), "pddl"_a)
      .def_property_readonly("root", &Planner::root);

  // BreadthFirstSearch
  py::class_<BreadthFirstSearch<Planner::Node>>(m, "BreadthFirstSearch")
      .def(py::init<const Planner::Node&, size_t, bool>(), "root"_a, "max_depth"_a, "verbose"_a = false)
      .def("__iter__", [](const BreadthFirstSearch<Planner::Node>& bfs) {
        return py::make_iterator(bfs.begin(), bfs.end());
      }, py::keep_alive<0, 1>());

  py::class_<DisjunctiveFormula>(m, "DisjunctiveFormula")
      .def_readwrite("conjunctions", &DisjunctiveFormula::conjunctions)
      .def("__repr__", [](const DisjunctiveFormula& dnf) {
        std::stringstream ss;
        ss << dnf;
        return ss.str();
      });

  py::class_<FormulaLiterals>(m, "FormulaLiterals")
      .def_readwrite("pos", &FormulaLiterals::pos)
      .def_readwrite("neg", &FormulaLiterals::neg);

  m.def("NormalizeConditions", &NormalizeConditions, "pddl"_a, "action"_a);

}

}  // namespace spatial_dyn
