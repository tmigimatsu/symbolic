/**
 * pddl.cc
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: March 7, 2020
 * Authors: Toki Migimatsu
 */

#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include "symbolic/pddl.h"

namespace symbolic {

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(symbolic, m) {

  // Articulated body
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

}

}  // namespace spatial_dyn
