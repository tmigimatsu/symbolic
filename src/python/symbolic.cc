/**
 * symbolic.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: March 7, 2020
 * Authors: Toki Migimatsu
 */

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>  // std::stringstream

#include "symbolic/normal_form.h"
#include "symbolic/pddl.h"
#include "symbolic/planning/breadth_first_search.h"
#include "symbolic/planning/planner.h"

namespace {

using ::symbolic::Pddl;
using ::symbolic::State;

State ParseState(const std::set<std::string>& str_state) {
  State state;
  for (const std::string& str_prop : str_state) {
    state.emplace(str_prop);
  }
  return state;
}

}  // namespace

namespace symbolic {

namespace py = pybind11;
using namespace pybind11::literals;  // NOLINT(google-build-using-namespace)

PYBIND11_MODULE(pysymbolic, m) {
  m.doc() = R"pbdoc(
    symbolic Python API
    ===================

    .. currentmodule:: symbolic

    .. autosummary::
       :toctree: _autosummary

       Pddl
       Object
       Action
       DisjunctiveFormula
  )pbdoc";

  // Pddl
  py::class_<Pddl>(m, "Pddl")
      .def(py::init<const std::string&, const std::string&>(), "domain"_a,
           "problem"_a)
      .def_property_readonly(
          "initial_state",
          [](const Pddl& pddl) { return Stringify(pddl.initial_state()); })
      .def_property_readonly("objects", &Pddl::objects)
      .def_property_readonly("actions", &Pddl::actions)
      .def_property_readonly("predicates", &Pddl::predicates)
      .def_property_readonly("axioms", &Pddl::axioms)
      .def_property_readonly("derived_predicates", &Pddl::derived_predicates)
      .def(
          "is_valid",
          [](const Pddl& pddl, bool verbose) { return pddl.IsValid(verbose); },
          "verbose"_a = false)
      .def("next_state",
           static_cast<std::set<std::string> (Pddl::*)(
               const std::set<std::string>&, const std::string&) const>(
               &Pddl::NextState))
      .def("derived_state",
           static_cast<std::set<std::string> (Pddl::*)(
               const std::set<std::string>&) const>(&Pddl::DerivedState))
      .def("consistent_state",
           static_cast<std::set<std::string> (Pddl::*)(
               const std::set<std::string>&) const>(&Pddl::ConsistentState))
      .def("is_valid_action",
           static_cast<bool (Pddl::*)(const std::set<std::string>&,
                                      const std::string&) const>(
               &Pddl::IsValidAction))
      .def("is_valid_state",
           static_cast<bool (Pddl::*)(const std::set<std::string>&) const>(
               &Pddl::IsValidState))
      .def("is_valid_tuple",
           static_cast<bool (Pddl::*)(
               const std::set<std::string>&, const std::string&,
               const std::set<std::string>&) const>(&Pddl::IsValidTuple))
      .def("is_goal_satisfied",
           static_cast<bool (Pddl::*)(const std::set<std::string>&) const>(
               &Pddl::IsGoalSatisfied))
      .def("is_valid_plan", &Pddl::IsValidPlan)
      .def("list_valid_arguments",
           static_cast<std::vector<std::vector<std::string>> (Pddl::*)(
               const std::set<std::string>&, const std::string&) const>(
               &Pddl::ListValidArguments))
      .def("list_valid_actions",
           static_cast<std::vector<std::string> (Pddl::*)(
               const std::set<std::string>&) const>(&Pddl::ListValidActions))
      .def(py::pickle(
          [](const Pddl& pddl) {
            return py::make_tuple(pddl.domain_pddl(), pddl.problem_pddl());
          },
          [](const py::tuple& domain_problem) {
            const auto domain = domain_problem[0].cast<std::string>();
            const auto problem = domain_problem[1].cast<std::string>();
            return Pddl(domain, problem);
          }));

  // Object::Type
  py::class_<Object::Type>(m, "ObjectType")
      .def("is_subtype",
           static_cast<bool (Object::Type::*)(const std::string&) const>(
               &Object::Type::IsSubtype))
      .def_property_readonly("name", &Object::Type::name)
      .def("__repr__", &Object::Type::name);

  // Object
  py::class_<Object>(m, "Object")
      .def_property_readonly("name", &Object::name)
      .def_property_readonly("type", &Object::type)
      .def("__repr__", &Object::name);

  // Action
  py::class_<Action>(m, "Action")
      .def_property_readonly("name", &Action::name)
      .def_property_readonly("parameters", &Action::parameters)
      .def_property_readonly("parameter_generator",
                             &Action::parameter_generator)
      .def("to_string",
           static_cast<std::string (Action::*)(
               const std::vector<Object>& arguments) const>(&Action::to_string))
      .def("__repr__",
           static_cast<std::string (Action::*)() const>(&Action::to_string));

  // Predicate
  py::class_<Predicate>(m, "Predicate")
      .def_property_readonly("name", &Predicate::name)
      .def_property_readonly("parameters", &Predicate::parameters)
      .def_property_readonly("parameter_generator",
                             &Predicate::parameter_generator)
      .def("to_string", static_cast<std::string (Predicate::*)(
                            const std::vector<Object>& arguments) const>(
                            &Predicate::to_string))
      .def("__repr__", static_cast<std::string (Predicate::*)() const>(
                           &Predicate::to_string));

  // Axiom
  py::class_<Axiom>(m, "Axiom")
      .def("is_consistent", &Axiom::IsConsistent)
      .def("__repr__", [](const Axiom& axiom) {
        std::stringstream ss;
        ss << axiom;
        return ss.str();
      });

  // DerivedPredicate
  py::class_<DerivedPredicate>(m, "DerivedPredicate")
      .def_static("apply",
                  [](const DerivedPredicate& pred) {

                  })
      .def("__repr__", [](const Axiom& axiom) {
        std::stringstream ss;
        ss << axiom;
        return ss.str();
      });

  // ParameterGenerator
  py::class_<ParameterGenerator>(m, "ParameterGenerator")
      .def("__getitem__", &ParameterGenerator::at)
      .def("__len__", &ParameterGenerator::size)
      .def("__nonzero__", &ParameterGenerator::empty)
      .def(
          "__iter__",
          [](ParameterGenerator& param_gen) {
            return py::make_iterator(param_gen.begin(), param_gen.end());
          },
          py::keep_alive<0, 1>());  // Keep vector alive while iterator is used

  // Planner::Node
  py::class_<Planner::Node>(m, "PlannerNode")
      .def_property_readonly("action", &Planner::Node::action)
      .def_property_readonly(
          "state",
          [](const Planner::Node& node) { return Stringify(node.state()); })
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
      .def(py::init<const Planner::Node&, size_t, bool>(), "root"_a,
           "max_depth"_a, "verbose"_a = false)
      .def(
          "__iter__",
          [](const BreadthFirstSearch<Planner::Node>& bfs) {
            return py::make_iterator(bfs.begin(), bfs.end());
          },
          py::keep_alive<0, 1>());

  py::class_<DisjunctiveFormula>(m, "DisjunctiveFormula")
      .def_readonly("conjunctions", &DisjunctiveFormula::conjunctions)
      .def_static("normalize_conditions",
                  &DisjunctiveFormula::NormalizeConditions)
      .def("__repr__", [](const DisjunctiveFormula& dnf) {
        std::stringstream ss;
        ss << dnf;
        return ss.str();
      });

  py::class_<FormulaLiterals>(m, "FormulaLiterals")
      .def_property_readonly(
          "pos", [](const FormulaLiterals& f) { return Stringify(f.pos); })
      .def_property_readonly(
          "neg", [](const FormulaLiterals& f) { return Stringify(f.neg); })
      .def(py::pickle(
          [](const FormulaLiterals& f) {
            return py::make_tuple(Stringify(f.pos), Stringify(f.neg));
          },
          [](const py::tuple& pos_neg) {
            return FormulaLiterals{
                ParseState(pos_neg[0].cast<std::set<std::string>>()),
                ParseState(pos_neg[1].cast<std::set<std::string>>())};
          }));

  m.def("NormalizeConditions", &DisjunctiveFormula::NormalizeConditions,
        "pddl"_a, "action"_a);
}

}  // namespace symbolic
