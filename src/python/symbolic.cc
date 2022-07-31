/**
 * symbolic.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: March 7, 2020
 * Authors: Toki Migimatsu
 */

#include <VAL/ptree.h>
#include <pybind11/eigen.h>
#include <pybind11/functional.h>
#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <exception>  // std::out_of_range
#include <sstream>    // std::stringstream

#include "symbolic/normal_form.h"
#include "symbolic/pddl.h"
#include "symbolic/planning/breadth_first_search.h"
#include "symbolic/planning/planner.h"

namespace {

using ::symbolic::Object;
using ::symbolic::Pddl;
using ::symbolic::Planner;
using ::symbolic::State;

using StringSet = std::set<std::string>;
using StringVector = std::vector<std::string>;

State ParseState(const Pddl& pddl, const StringSet& str_state) {
  State state;
  state.reserve(str_state.size());
  for (const std::string& str_prop : str_state) {
    state.emplace(pddl, str_prop);
  }
  return state;
}

std::vector<Object> ParseObjects(const Pddl& pddl,
                                 const StringVector& str_objects) {
  std::vector<Object> objects;
  objects.reserve(str_objects.size());
  for (const std::string& str_object : str_objects) {
    objects.emplace_back(pddl, str_object);
  }
  return objects;
}

struct BreadthFirstSearch {
  BreadthFirstSearch(const Planner::Node& root, size_t max_depth, bool verbose,
                     double timeout)
      : bfs(root, max_depth, verbose,
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::duration<double>(timeout))) {}

  ::symbolic::BreadthFirstSearch<Planner::Node> bfs;
  ::symbolic::BreadthFirstSearch<Planner::Node>::iterator it;
  bool initialized = false;
};

}  // namespace

namespace symbolic {

namespace py = pybind11;
using namespace pybind11::literals;  // NOLINT(google-build-using-namespace)

PYBIND11_MODULE(pysymbolic, m) {
  m.doc() = R"pbdoc(
    symbolic Python API
    ===================

    Python wrapper for the symbolic library.

    .. currentmodule:: symbolic
  )pbdoc";

  // Pddl
  py::class_<Pddl>(m, "Pddl")
      .def(py::init<const std::string&, const std::string&, bool>(), "domain"_a,
           "problem"_a, "apply_axioms"_a = true, R"pbdoc(
             Parse the pddl specification from the domain and problem files.

             Args:
                 domain: Path to the domain pddl.
                 problem: Path to the problem pddl.
                 apply_axioms: Whether to apply axioms to the initial state.

             Example:
                 >>> import symbolic
                 >>> symbolic.Pddl("../resources/domain.pddl", "../resources/problem.pddl")
                 symbolic.Pddl('../resources/domain.pddl', '../resources/problem.pddl')

             .. seealso:: C++: :symbolic:`symbolic::Pddl::Pddl`.
            )pbdoc")
      .def(py::init<const std::string&>(), "domain"_a, R"pbdoc(
             Parse the pddl specification from the domain file without a problem.

             Args:
                 domain: Path to the domain pddl.

             Example:
                 >>> import symbolic
                 >>> symbolic.Pddl("../resources/domain.pddl")
                 symbolic.Pddl('../resources/domain.pddl', '')

             .. seealso:: C++: :symbolic:`symbolic::Pddl::Pddl`.
            )pbdoc")
      .def(
          "is_valid",
          [](const Pddl& pddl, bool verbose) { return pddl.IsValid(verbose); },
          "verbose"_a = false, R"pbdoc(
            Evaluate whether the pddl specification is valid using VAL.

            Args:
                verbose: Print diagnostic information.
            Returns:
                Whether the pddl specification is valid.

            Example:
                >>> import symbolic
                >>> pddl = symbolic.Pddl("../resources/domain.pddl", "../resources/problem.pddl")
                >>> pddl.is_valid()
                True

            .. seealso:: C++: :symbolic:`symbolic::Pddl::IsValid`.
          )pbdoc")
      .def(
          "next_state",
          [](const Pddl& pddl, const std::unordered_set<std::string>& state,
             const std::string& action) {
            return pddl.NextState(State(pddl, state), action).Stringify();
          },
          "state"_a, "action"_a, R"pbdoc(
            Apply an action to the given state.

            The action's preconditions are not checked. The resulting state includes
            derived predicates.

            Args:
                state: Current state.
                action: Action call in the form of :code:`"action(obj_a, obj_b)"`.
            Returns:
                Next state.

            Example:
                >>> import symbolic
                >>> pddl = symbolic.Pddl("../resources/domain.pddl", "../resources/problem.pddl")
                >>> next_state = pddl.next_state(pddl.initial_state, "pick(hook)")
                >>> sorted(next_state)
                ['inhand(hook)', 'inworkspace(hook)', 'inworkspace(shelf)', 'inworkspace(table)', 'on(box, table)']

            .. seealso:: C++: :symbolic:`symbolic::Pddl::NextState`.
          )pbdoc")
      .def(
          "apply_actions",
          [](const Pddl& pddl, const std::unordered_set<std::string>& state,
             const std::vector<std::string>& actions) {
            return pddl.ApplyActions(State(pddl, state), actions).Stringify();
          },
          "state"_a, "action"_a, R"pbdoc(
            Execute a sequence of actions from the given state

            The action preconditions are not checked. The resulting state includes
            derived predicates.

            Args:
                state: Current state.
                actions: Action calls in the form of :code:`"action(obj_a, obj_b)"`.
            Returns:
                Final state.

            .. seealso:: C++: :symbolic:`symbolic::Pddl::Execute`.
          )pbdoc")
      .def(
          "derived_state",
          [](const Pddl& pddl, const std::unordered_set<std::string>& state) {
            return pddl.DerivedState(State(pddl, state)).Stringify();
          },
          "state"_a)
      .def(
          "consistent_state",
          [](const Pddl& pddl, const std::unordered_set<std::string>& state) {
            return pddl.ConsistentState(State(pddl, state)).Stringify();
          },
          "state"_a, R"pbdoc(
            Applies the axioms to the given state.

            Args:
                state: Current state.
            Returns:
                Updated state.

            .. seealso:: C++: :symbolic:`symbolic::Pddl::IsValidState`.
          )pbdoc")
      .def(
          "consistent_state",
          [](const Pddl& pddl, const std::unordered_set<std::string>& state_pos,
             const std::unordered_set<std::string>& state_neg) {
            return pddl
                .ConsistentState(PartialState(pddl, state_pos, state_neg))
                .Stringify();
          },
          "state_pos"_a, "state_neg"_a, R"pbdoc(
            Applies the axioms to the given partial state.

            Args:
                state_pos: Positive propositions in partial state.
                state_pos: Negative propositions in negative state.
            Returns:
                Updated state.

            .. seealso:: C++: :symbolic:`symbolic::Pddl::IsValidState`.
          )pbdoc")
      .def(
          "is_valid_action",
          [](const Pddl& pddl, const std::unordered_set<std::string>& state,
             const std::string& action) {
            return pddl.IsValidAction(State(pddl, state), action);
          },
          "state"_a, "action"_a, R"pbdoc(
            Evaluates whether the action's preconditions are satisfied.

            Args:
                state: Current state.
                action: Action call in the form of :code:`"action(obj_a, obj_b)"`.
            Returns:
                Whether the action can be applied to the state.

            Example:
                >>> import symbolic
                >>> pddl = symbolic.Pddl("../resources/domain.pddl", "../resources/problem.pddl")
                >>> pddl.is_valid_action(pddl.initial_state, "pick(hook)")
                True
                >>> pddl.is_valid_action(pddl.initial_state, "pick(box)")
                False

            .. seealso:: C++: :symbolic:`symbolic::Pddl::IsValidAction`.
          )pbdoc")
      .def(
          "is_valid_state",
          [](const Pddl& pddl, const std::unordered_set<std::string>& state) {
            return pddl.IsValidState(State(pddl, state));
          },
          "state"_a, R"pbdoc(
            Evaluates whether the state satisfies the axioms.

            Args:
                state: Current state.
            Returns:
                Whether the state is valid.

            .. seealso:: C++: :symbolic:`symbolic::Pddl::IsValidState`.
          )pbdoc")
      .def(
          "is_valid_state",
          [](const Pddl& pddl, const std::unordered_set<std::string>& state_pos,
             const std::unordered_set<std::string>& state_neg) {
            return pddl.IsValidState(PartialState(pddl, state_pos, state_neg));
          },
          "state_pos"_a, "state_neg"_a, R"pbdoc(
            Evaluate whether the partial state satisfies the axioms.

            Args:
                state_pos: Positive propositions in partial state.
                state_neg: Negative propositions in partial state.
            Returns:
                Whether the partial state is valid.

            .. seealso:: C++: :symbolic:`symbolic::Pddl::IsValidState`.
          )pbdoc")
      .def("add_object", &Pddl::AddObject, "name"_a, "type"_a)
      .def("remove_object", &Pddl::RemoveObject, "name"_a)
      .def_property_readonly("name", &Pddl::name, R"pbdoc(
          Pddl domain name.

          :type: str
      )pbdoc")
      .def_property(
          "initial_state",
          [](const Pddl& pddl) { return Stringify(pddl.initial_state()); },
          [](Pddl& pddl, const std::unordered_set<std::string>& state) {
            pddl.set_initial_state(State(pddl, state));
          },
          R"pbdoc(
            Initial state for planning.
          )pbdoc")
      .def_property_readonly("object_map", &Pddl::object_map)
      .def_property_readonly("constants", &Pddl::constants)
      .def_property_readonly("objects", &Pddl::objects)
      .def_property_readonly("actions", &Pddl::actions)
      .def_property_readonly("predicates", &Pddl::predicates)
      .def_property_readonly("axioms", &Pddl::axioms)
      .def_property_readonly("derived_predicates", &Pddl::derived_predicates)
      .def_property_readonly("state_index", &Pddl::state_index)
      .def_property_readonly("goal", &Pddl::goal)
      .def("is_valid_tuple",
           static_cast<bool (Pddl::*)(const StringSet&, const std::string&,
                                      const StringSet&) const>(
               &Pddl::IsValidTuple))
      .def("is_goal_satisfied",
           static_cast<bool (Pddl::*)(const std::set<std::string>&) const>(
               &Pddl::IsGoalSatisfied))
      .def("is_valid_plan", &Pddl::IsValidPlan)
      .def("list_valid_arguments",
           static_cast<std::vector<StringVector> (Pddl::*)(
               const StringSet&, const std::string&) const>(
               &Pddl::ListValidArguments))
      .def("list_valid_actions",
           static_cast<StringVector (Pddl::*)(const StringSet&) const>(
               &Pddl::ListValidActions))
      .def_property_readonly("domain_pddl", &Pddl::domain_pddl)
      .def_property_readonly("problem_pddl", &Pddl::domain_pddl)
      .def("__repr__",
           [](const Pddl& pddl) {
             return "symbolic.Pddl('" + pddl.domain_pddl() + "', '" +
                    pddl.problem_pddl() + "')";
           })
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
      .def_static("parse", &Action::Parse, "pddl"_a, "action_call"_a)
      .def("to_string",
           [](const Action& action,
              const std::vector<std::string>& arguments) -> std::string {
             return action.to_string(ParseObjects(action.pddl(), arguments));
           })
      .def("__repr__",
           static_cast<std::string (Action::*)() const>(&Action::to_string));

  // Predicate
  py::class_<Predicate>(m, "Predicate")
      .def_property_readonly("name", &Predicate::name, R"pbdoc(
          Predicate head.

          :type: str

          .. seealso:: C++: :symbolic:`symbolic::Predicate::name`.
      )pbdoc")
      .def_property_readonly("parameters", &Predicate::parameters, R"pbdoc(
          List of predicate parameters.

          :type: `symbolic.Object`

          .. seealso:: C++: :symbolic:`symbolic::Predicate::parameters`.
      )pbdoc")
      .def_property_readonly("parameter_generator",
                             &Predicate::parameter_generator, R"pbdoc(
          Combination generator for predicate parameters.

          :type: `symbolic.ParameterGenerator`

          .. seealso:: C++: :symbolic:`symbolic::Predicate::parameter_generator`.
       )pbdoc")
      .def(
          "to_string",
          [](const Predicate& pred,
             const std::vector<std::string>& arguments) -> std::string {
            return pred.to_string(ParseObjects(pred.pddl(), arguments));
          },
          R"pbdoc(
          Creates a string representation of the predicate with the given arguments.

          .. seealso:: C++: :symbolic:`symbolic::Predicate::to_string`.
      )pbdoc")
      .def("__repr__",
           static_cast<std::string (Predicate::*)() const>(
               &Predicate::to_string),
           R"pbdoc(
          Creates a string representation of the predicate with the default parameters.

          .. seealso:: C++: :symbolic:`symbolic::Predicate::to_string`.
       )pbdoc");

  // Axiom
  py::class_<Axiom>(m, "Axiom")
      .def("is_consistent",
           static_cast<bool (Axiom::*)(const State&) const>(
               &Axiom::IsConsistent),
           "state"_a, R"pbdoc(
          Determine whether the axiom is satisfied in the given state.

          Args:
              state: State to evaluate.
          Returns:
              Whether the state is consistent with this axiom.
          )pbdoc")
      .def("is_consistent",
           static_cast<bool (Axiom::*)(const PartialState&) const>(
               &Axiom::IsConsistent),
           "state"_a, R"pbdoc(
          Determine whether the axiom is satisfied in the given partial state.

          Args:
              state: Partial state to evaluate.
          Returns:
              Whether the state is consistent with this axiom.
          )pbdoc")
      // .def_static(
      //     "is_consistent_all",
      //     static_cast<bool (*)(const std::vector<Axiom>&, const
      //     PartialState&)>(
      //         &Axiom::IsConsistent),
      //     "axioms"_a, "state"_a, R"pbdoc(
      //     Determine whether all axioms are satisfied in the given partial
      //     state.

      //     Returns false only if a partial state fully satisfies the
      //     pre-conditions of the axiom and explicitly does not satisfy the
      //     post-conditions. If a proposition in the partial state is unknown,
      //     the axiom is assumed to be satisfied.

      //     Args:
      //         state: Partial state to evaluate.
      //     Returns:
      //         Whether the state is consistent with this axiom.
      //     )pbdoc")
      .def("__repr__", [](const Axiom& axiom) {
        std::stringstream ss;
        ss << axiom;
        return ss.str();
      });

  // DerivedPredicate
  py::class_<DerivedPredicate>(m, "DerivedPredicate")
      .def_static("apply",
                  [](const DerivedPredicate& pred) {
                    // TODO
                  })
      .def("__repr__", [](const Axiom& axiom) {
        std::stringstream ss;
        ss << axiom;
        return ss.str();
      });

  // Formula
  py::class_<Formula>(m, "Formula").def("__repr__", [](const Formula& formula) {
    return formula.to_string();
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
          py::keep_alive<0, 1>())  // Keep vector alive while iterator is used
      .def("index",
           [](const ParameterGenerator& param_gen,
              const StringVector& str_args) -> size_t {
             return param_gen.find(
                 Object::ParseArguments(param_gen.pddl(), str_args));
           });

  // StateIndex
  py::class_<StateIndex>(m, "StateIndex")
      .def("get_proposition",
           [](const StateIndex& state_index, int idx_proposition) {
             if (idx_proposition < 0) idx_proposition += state_index.size();
             return state_index.GetProposition(idx_proposition).to_string();
           })
      .def("get_proposition_index",
           [](const StateIndex& state_index, const std::string& str_prop) {
             return state_index.GetPropositionIndex(
                 Proposition(state_index.pddl(), str_prop));
           })
      .def("get_state",
           [](const StateIndex& state_index,
              // NOLINTNEXTLINE(performance-unnecessary-value-param)
              Eigen::Ref<const StateIndex::IndexedState> indexed_state) {
             return Stringify(state_index.GetState(indexed_state));
           })
      .def("get_indexed_state",
           [](const StateIndex& state_index, const StringSet& str_state) {
             return state_index.GetIndexedState(
                 ParseState(state_index.pddl(), str_state));
           })
      .def("__len__", &StateIndex::size, R"pbdoc(
          Size of the state index.

          .. seealso:: C++: :symbolic:`symbolic::StateIndex::size`.
      )pbdoc");
  // .def(
  //     "__iter__",
  //     [](const StateIndex& state_index) {
  //     // TODO: Iterator returns a Proposition, need to convert to string.
  //       return py::make_iterator(state_index.begin(), state_index.end());
  //     },
  //     R"pbdoc(
  //     Iterate over the state index.

  //     .. seealso:: C++: :symbolic:`symbolic::StateIndex::iterator`.
  // )pbdoc");

  // Planner::Node
  py::class_<Planner::Node>(m, "PlannerNode")
      .def(py::init<const Planner::Node&>())
      .def_property_readonly("action", &Planner::Node::action)
      .def_property_readonly(
          "state",
          [](const Planner::Node& node) { return Stringify(node.state()); })
      .def_property_readonly("depth", &Planner::Node::depth)
      .def(
          "__iter__",
          [](const Planner::Node& node) {
            return py::make_iterator(node.begin(), node.end());
          },
          py::keep_alive<0, 1>())
      .def("__repr__", [](const Planner::Node& node) {
        std::stringstream ss;
        ss << node;
        return ss.str();
      });

  // Planner
  py::class_<Planner>(m, "Planner")
      .def(py::init<const Pddl&>(), "pddl"_a, R"pbdoc(
        Planner class to find a state that satisfies the goal condition from the initial state.

        Args:
          pddl: Pddl instance.

        .. seealso:: C++: :symbolic:`symbolic::Planner::Planner`.
       )pbdoc")
      .def(py::init([](const Pddl& pddl, const StringSet& state) {
             return Planner(pddl, ParseState(pddl, state));
           }),
           "pddl"_a, "state"_a, R"pbdoc(
        Planner class to find a state that satisfies the goal condition from the given state.

        Args:
          pddl: Pddl instance.
          state: State from which to search.

        .. seealso:: C++: :symbolic:`symbolic::Planner::Planner`.
       )pbdoc")
      .def_property_readonly("root", &Planner::root);

  // BreadthFirstSearch
  // py::class_<BreadthFirstSearch<Planner::Node>>(m, "BreadthFirstSearch")
  //     .def(py::init<const Planner::Node&, size_t, bool>(), "root"_a,
  //          "max_depth"_a, "verbose"_a = false)
  //     .def(
  //         "__iter__",
  //         [](const BreadthFirstSearch<Planner::Node>& bfs) {
  //           return BreadthFirstSearch<Planner::Node>::iterator(&bfs);
  //         },
  //         py::keep_alive<0, 1>());
  // .def(
  //     "__iter__",
  //     [](const BreadthFirstSearch<Planner::Node>& bfs) {
  //       return py::make_iterator(bfs.begin(), bfs.end());
  //     },
  //     py::keep_alive<0, 1>());
  py::class_<::BreadthFirstSearch>(m, "BreadthFirstSearch")
      .def(py::init<const Planner::Node&, size_t, bool, double>(), "root"_a,
           "max_depth"_a, "verbose"_a = false, "timeout"_a = 0)
      .def("__iter__", [](::BreadthFirstSearch& it) { return it; })
      .def("__next__", [](::BreadthFirstSearch& it) {
        if (!it.initialized) {
          it.it = it.bfs.begin();
          it.initialized = true;
        }

        if (it.it == it.bfs.end()) {
          throw pybind11::stop_iteration();
        }

        return *it.it;
      });

  py::class_<DisjunctiveFormula>(m, "DisjunctiveFormula")
      .def_readonly("conjunctions", &DisjunctiveFormula::conjunctions)
      .def_static("normalize_goal", &DisjunctiveFormula::NormalizeGoal,
                  "pddl"_a, "apply_axioms"_a = false)
      .def_static("normalize_conditions",
                  &DisjunctiveFormula::NormalizeConditions, "pddl"_a,
                  "action_call"_a, "apply_axioms"_a = false, R"pbdoc(
          Normalize the pre/post conditions of the given action.

          If either of the conditions are invalid, this function will return an
          empty optional.

          Args:
              pddl: Pddl object.
              action: Action call in the form of :code:`"action(obj_a, obj_b)"`.
              apply_axioms: Whether to apply the axioms to the pre/post conditions.
          Returns:
              Pair of normalized pre/post conditions.

          Example:
              >>> import symbolic
              >>> pddl = symbolic.Pddl("../resources/domain.pddl", "../resources/problem.pddl")
              >>> symbolic.DisjunctiveFormula.normalize_conditions(pddl, "pick(hook)")
              ((or
                  (and
                      inworkspace(hook)
                      not inhand(hook)
                      not inhand(box)
                  )
              )
              , (or
                  (and
                      inhand(hook)
                      not on(hook, shelf)
                      not on(hook, box)
                      not on(hook, hook)
                      not on(hook, table)
                  )
              )
              )

          .. seealso:: C++: :symbolic:`symbolic::DisjunctiveFormula::NormalizeConditions`.
          )pbdoc")
      .def_static("normalize_preconditions",
                  &DisjunctiveFormula::NormalizePreconditions, "pddl"_a,
                  "action_call"_a, "apply_axioms"_a = false)
      .def_static("normalize_postconditions",
                  &DisjunctiveFormula::NormalizePostconditions, "pddl"_a,
                  "action_call"_a, "apply_axioms"_a = false)
      .def("__repr__", [](const DisjunctiveFormula& dnf) {
        std::stringstream ss;
        ss << dnf;
        return ss.str();
      });

  py::class_<PartialState>(m, "PartialState")
      .def_property_readonly(
          "pos", [](const PartialState& s) { return Stringify(s.pos()); })
      .def_property_readonly(
          "neg", [](const PartialState& s) { return Stringify(s.neg()); })
      // .def(py::pickle(
      //     [](const PartialState& s) {
      //       return py::make_tuple(Stringify(s.pos()), Stringify(s.neg()));
      //     },
      //     [](const py::tuple& pos_neg) {
      //       // TODO: Construct with pddl.
      //       return PartialState{ParseState(pos_neg[0].cast<StringSet>()),
      //                           ParseState(pos_neg[1].cast<StringSet>())};
      //     }))
      .def("__repr__", [](const PartialState& s) {
        std::stringstream ss;
        ss << s;
        return ss.str();
      });

  py::add_ostream_redirect(m);
}

}  // namespace symbolic
