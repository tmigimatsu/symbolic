/**
 * pddl.cc
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#include "symbolic/pddl.h"

#include <VAL/FlexLexer.h>
#include <VAL/ptree.h>
#include <VAL/typecheck.h>

#include <fstream>  // std::ifstream
#include <sstream>  // std::stringstream
#include <string>   // std::string
#include <utility>  // std::move

#include "symbolic/utils/parameter_generator.h"
#include "utils/doctest.h"

extern int yyparse();
extern int yydebug;

namespace VAL_v1 {

// Expected in pddl+.cpp
parse_category* top_thing = nullptr;
analysis* current_analysis = nullptr;
yyFlexLexer* yfl = nullptr;

// Expected in typechecker.cpp
bool Verbose = false;
std::ostream* report = nullptr;

std::ostream& operator<<(std::ostream& os, const domain& domain);

std::ostream& operator<<(std::ostream& os, const problem& problem);

std::ostream& operator<<(std::ostream& os, const simple_effect& effect);

std::ostream& operator<<(std::ostream& os, const var_symbol_list& args);

std::ostream& operator<<(std::ostream& os, const parameter_symbol_list& args);

}  // namespace VAL_v1

const char* current_filename = nullptr;  // Expected in parse_error.h

namespace {

std::unique_ptr<VAL_v1::analysis> ParsePddl(
    const std::string& filename_domain, const std::string& filename_problem) {
  std::unique_ptr<VAL_v1::analysis> analysis =
      std::make_unique<VAL_v1::analysis>();
  yyFlexLexer yfl;

  VAL_v1::current_analysis = analysis.get();
  VAL_v1::yfl = &yfl;
  yydebug = 0;  // Set to 1 to output yacc trace

  // Parse domain
  current_filename = filename_domain.c_str();
  std::ifstream pddl_domain(filename_domain);
  yfl.switch_streams(&pddl_domain, &std::cout);
  yyparse();
  if (analysis->the_domain == nullptr) {
    throw std::runtime_error("ParsePddl(): Unable to parse domain from file: " +
                             std::string(filename_domain));
  }

  // Parse problem
  current_filename = filename_problem.c_str();
  std::ifstream pddl_problem(filename_problem);
  yfl.switch_streams(&pddl_problem, &std::cout);
  yyparse();
  if (analysis->the_problem == nullptr) {
    throw std::runtime_error(
        "ParsePddl(): Unable to parse problem from file: " +
        std::string(filename_problem));
  }

  return analysis;
}

using ::symbolic_v1::Action;
using ::symbolic_v1::Axiom;
using ::symbolic_v1::DerivedPredicate;
using ::symbolic_v1::Object;
using ::symbolic_v1::PartialState;
using ::symbolic_v1::Pddl;
using ::symbolic_v1::Predicate;
using ::symbolic_v1::Proposition;
using ::symbolic_v1::State;

State ParseState(const Pddl& pddl, const std::set<std::string>& str_state) {
  State state;
  state.reserve(str_state.size());
  for (const std::string& str_prop : str_state) {
    state.emplace(pddl, str_prop);
  }
  return state;
}
PartialState ParseState(const Pddl& pddl,
                        const std::set<std::string>& str_state_pos,
                        const std::set<std::string>& str_state_neg) {
  return PartialState(ParseState(pddl, str_state_pos),
                      ParseState(pddl, str_state_neg));
}

std::vector<Object> GetObjects(const VAL_v1::domain& domain,
                               const VAL_v1::problem& problem) {
  std::vector<Object> objects =
      Object::CreateList(domain.types, domain.constants);
  const std::vector<Object> objects_2 =
      Object::CreateList(domain.types, problem.objects);
  objects.insert(objects.end(), objects_2.begin(), objects_2.end());
  return objects;
}

std::unordered_map<std::string, std::vector<Object>> CreateObjectTypeMap(
    const std::vector<Object>& objects) {
  std::unordered_map<std::string, std::vector<Object>> object_map;
  for (const Object& object : objects) {
    std::vector<std::string> types = object.type().ListTypes();
    for (const std::string& type : types) {
      object_map[type].push_back(object);
    }
  }
  return object_map;
}

std::vector<Action> GetActions(const Pddl& pddl, const VAL_v1::domain& domain) {
  std::vector<Action> actions;
  for (const VAL_v1::operator_* op : *domain.ops) {
    const auto* a = dynamic_cast<const VAL_v1::action*>(op);
    if (a == nullptr) continue;
    actions.emplace_back(pddl, op);
  }
  return actions;
}

std::vector<Predicate> GetPredicates(const Pddl& pddl,
                                     const VAL_v1::domain& domain) {
  std::vector<Predicate> predicates;
  for (const VAL_v1::pred_decl* pred : *domain.predicates) {
    predicates.emplace_back(pddl, pred);
  }
  return predicates;
}

std::vector<Axiom> GetAxioms(const Pddl& pddl, const VAL_v1::domain& domain) {
  std::vector<Axiom> axioms;
  for (const VAL_v1::operator_* op : *domain.ops) {
    const auto* a = dynamic_cast<const VAL_v1::axiom*>(op);
    if (a == nullptr) continue;
    axioms.emplace_back(pddl, op);
  }
  return axioms;
}

std::vector<DerivedPredicate> GetDerivedPredicates(
    const Pddl& pddl, const VAL_v1::domain& domain) {
  std::vector<DerivedPredicate> predicates;
  predicates.reserve(domain.drvs->size());
  for (const VAL_v1::derivation_rule* drv : *domain.drvs) {
    predicates.emplace_back(pddl, drv);
  }
  return predicates;
}

State GetInitialState(const VAL_v1::domain& domain,
                      const VAL_v1::problem& problem) {
  State initial_state;
  for (const VAL_v1::simple_effect* effect :
       problem.initial_state->add_effects) {
    std::vector<Object> arguments;
    arguments.reserve(effect->prop->args->size());
    for (const VAL_v1::parameter_symbol* arg : *effect->prop->args) {
      arguments.emplace_back(domain.types, arg);
    }
    initial_state.emplace(effect->prop->head->getName(), std::move(arguments));
  }
  // for (const Object& object : objects) {
  //   initial_state.emplace("=", std::vector<Object>{object, object});
  // }
  return initial_state;
}

State Apply(const State& state, const Action& action,
            const std::vector<Object>& arguments,
            const std::vector<DerivedPredicate>& predicates) {
  State next_state = action.Apply(state, arguments);
  DerivedPredicate::Apply(predicates, &next_state);
  return next_state;
}

bool Apply(const Action& action, const std::vector<Object>& arguments,
           const std::vector<DerivedPredicate>& predicates, State* state) {
  bool is_changed = action.Apply(arguments, state);
  is_changed |= DerivedPredicate::Apply(predicates, state);
  return is_changed;
}

}  // namespace

namespace symbolic_v1 {

// Empty destructor needs to be implemented here because VAL_v1::analysis is not
// fully defined in the header.
Pddl::~Pddl() {}

Pddl::Pddl(const std::string& domain_pddl, const std::string& problem_pddl)
    : analysis_(ParsePddl(domain_pddl, problem_pddl)),
      domain_pddl_(domain_pddl),
      problem_pddl_(problem_pddl),
      objects_(GetObjects(*analysis_->the_domain, *analysis_->the_problem)),
      object_map_(CreateObjectTypeMap(objects_)),
      actions_(GetActions(*this, *analysis_->the_domain)),
      predicates_(GetPredicates(*this, *analysis_->the_domain)),
      axioms_(GetAxioms(*this, *analysis_->the_domain)),
      derived_predicates_(GetDerivedPredicates(*this, *analysis_->the_domain)),
      state_index_(predicates_),
      initial_state_(
          GetInitialState(*analysis_->the_domain, *analysis_->the_problem)),
      goal_(*this, analysis_->the_problem->the_goal) {}

bool Pddl::IsValid(bool verbose, std::ostream& os) const {
  VAL_v1::Verbose = verbose;
  VAL_v1::report = &os;

  VAL_v1::TypeChecker tc(analysis_.get());
  const bool is_domain_valid = tc.typecheckDomain();
  const bool is_problem_valid = tc.typecheckProblem();

  // Output the errors from all input files
  if (verbose) {
    analysis_->error_list.report();
  }

  return is_domain_valid && is_problem_valid;
}

TEST_CASE_FIXTURE(testing::Fixture, "Pddl.IsValid") { REQUIRE(pddl.IsValid()); }

State Pddl::NextState(const State& state,
                      const std::string& action_call) const {
  // Parse strings
  const std::pair<Action, std::vector<Object>> action_args =
      Action::Parse(*this, action_call);
  const Action& action = action_args.first;
  const std::vector<Object>& arguments = action_args.second;

  return Apply(state, action, arguments, derived_predicates());
}

TEST_CASE_FIXTURE(testing::Fixture, "Pddl.NextState") {
  const State& state = pddl.initial_state();
  State next_state = state;
  next_state.erase(Proposition(pddl, "on(hook, table)"));
  next_state.emplace(pddl, "inhand(hook)");
  REQUIRE(pddl.NextState(state, "pick(hook)") == next_state);
}

State Pddl::DerivedState(const State& state) const {
  return DerivedPredicate::Apply(state, derived_predicates());
}

State Pddl::ConsistentState(const State& state) const {
  State next_state = state;
  bool is_changed = true;
  while (is_changed) {
    is_changed = false;
    for (const Axiom& axiom : axioms()) {
      is_changed |= axiom.Apply(&next_state);
    }
  }
  return next_state;
}

PartialState Pddl::ConsistentState(const PartialState& state) const {
  constexpr int kMaxIterations = 50;
  PartialState next_state = state;

  size_t i = 0;
  bool is_changed = true;
  while (is_changed) {
    is_changed = false;
    for (const Axiom& axiom : axioms()) {
      const int axiom_change = axiom.Apply(&next_state);
      is_changed |= axiom_change > 0;

      if (axiom_change == 2) {
        std::stringstream ss;
        ss << "Pddl::ConsistentState(): Axiom violation" << std::endl
           << axiom << std::endl
           << std::endl
           << next_state << std::endl;
        throw std::runtime_error(ss.str());
      }
    }

    if (i++ > kMaxIterations) {
      throw std::runtime_error(
          "Pddl::ConsistentState(): Exceeded max num iterations.");
    }
  }
  return next_state;
}

bool Pddl::IsValidAction(const State& state,
                         const std::string& action_call) const {
  // Parse strings
  const std::pair<Action, std::vector<Object>> action_args =
      Action::Parse(*this, action_call);
  const Action& action = action_args.first;
  const std::vector<Object>& arguments = action_args.second;

  return action.IsValid(state, arguments);
}

TEST_CASE_FIXTURE(testing::Fixture, "Pddl.IsValidAction") {
  const State& state = pddl.initial_state();
  REQUIRE(pddl.IsValidAction(pddl.initial_state(), "pick(hook)") == true);
  REQUIRE(pddl.IsValidAction(pddl.initial_state(), "pick(box)") == false);
}

bool Pddl::IsValidState(const State& state) const {
  for (const Axiom& axiom : axioms()) {
    if (!axiom.IsConsistent(state)) return false;
  }
  return true;
}

bool Pddl::IsValidState(const PartialState& state) const {
  return Axiom::IsConsistent(axioms(), state);
}

bool Pddl::IsValidTuple(const State& state, const std::string& action_call,
                        const State& next_state) const {
  // Parse strings
  const std::pair<Action, std::vector<Object>> action_args =
      Action::Parse(*this, action_call);
  const Action& action = action_args.first;
  const std::vector<Object>& arguments = action_args.second;

  return action.IsValid(state, arguments) &&
         Apply(state, action, arguments, derived_predicates()) == next_state;
}
bool Pddl::IsValidTuple(const std::set<std::string>& str_state,
                        const std::string& action_call,
                        const std::set<std::string>& str_next_state) const {
  // Parse strings
  const State state = ParseState(*this, str_state);
  const State next_state = ParseState(*this, str_next_state);
  return IsValidTuple(state, action_call, next_state);
}

bool Pddl::IsGoalSatisfied(const std::set<std::string>& str_state) const {
  // Parse strings
  const State state = ParseState(*this, str_state);

  return goal_(state);
}

bool Pddl::IsValidPlan(const std::vector<std::string>& action_skeleton) const {
  State state = initial_state_;
  for (const std::string& action_call : action_skeleton) {
    // Parse strings
    std::pair<Action, std::vector<Object>> action_args =
        Action::Parse(*this, action_call);
    const Action& action = action_args.first;
    const std::vector<Object>& arguments = action_args.second;

    if (!action.IsValid(state, arguments)) return false;
    Apply(action, arguments, derived_predicates(), &state);
  }
  return goal_(state);
}

std::vector<std::vector<Object>> Pddl::ListValidArguments(
    const State& state, const Action& action) const {
  std::vector<std::vector<Object>> arguments;
  ParameterGenerator param_gen(object_map(), action.parameters());
  for (const std::vector<Object>& args : param_gen) {
    if (action.IsValid(state, args)) arguments.push_back(args);
  }
  return arguments;
}
std::vector<std::vector<std::string>> Pddl::ListValidArguments(
    const std::set<std::string>& str_state,
    const std::string& action_name) const {
  // Parse strings
  const State state = ParseState(*this, str_state);
  const Action action(*this, action_name);
  const std::vector<std::vector<Object>> arguments =
      ListValidArguments(state, action);
  return Stringify(arguments);
}

// std::set<std::string> Pddl::initial_state_str() const {
//   return StringifyState(initial_state_);
// }

// std::vector<std::string> Pddl::actions_str() const {
//   return StringifyActions(actions_);
// }

std::vector<std::string> Pddl::ListValidActions(const State& state) const {
  std::vector<std::string> actions;
  for (const Action& action : actions_) {
    const std::vector<std::vector<Object>> arguments =
        ListValidArguments(state, action);
    for (const std::vector<Object>& args : arguments) {
      actions.emplace_back(action.to_string(args));
    }
  }
  return actions;
}

std::vector<std::string> Pddl::ListValidActions(
    const std::set<std::string>& state) const {
  return ListValidActions(ParseState(*this, state));
}

const std::string& Pddl::name() const { return symbol()->the_domain->name; }

std::set<std::string> Stringify(const State& state) {
  std::set<std::string> str_state;
  for (const Proposition& prop : state) {
    str_state.emplace(prop.to_string());
  }
  return str_state;
}
std::pair<std::set<std::string>, std::set<std::string>> Stringify(
    const PartialState& state) {
  return std::make_pair(Stringify(state.pos()), Stringify(state.neg()));
}

std::vector<std::string> Stringify(const std::vector<Action>& actions) {
  std::vector<std::string> str_actions;
  str_actions.reserve(actions.size());
  for (const Action& action : actions) {
    str_actions.emplace_back(action.name());
  }
  return str_actions;
}

std::vector<std::vector<std::string>> Stringify(
    const std::vector<std::vector<Object>>& arguments) {
  std::vector<std::vector<std::string>> str_arguments;
  str_arguments.reserve(arguments.size());
  for (const std::vector<Object>& args : arguments) {
    std::vector<std::string> str_args;
    str_args.reserve(args.size());
    for (const Object& arg : args) {
      str_args.emplace_back(arg.name());
    }
    str_arguments.emplace_back(std::move(str_args));
  }
  return str_arguments;
}

std::vector<std::string> Stringify(const std::vector<Object>& objects) {
  std::vector<std::string> str_objects;
  str_objects.reserve(objects.size());
  for (const Object& object : objects) {
    str_objects.emplace_back(object.name());
  }
  return str_objects;
}

std::ostream& operator<<(std::ostream& os, const Pddl& pddl) {
  os << *pddl.symbol()->the_domain << std::endl;
  os << *pddl.symbol()->the_domain << std::endl;
  return os;
}

}  // namespace symbolic_v1

namespace {

void PrintGoal(std::ostream& os, const VAL_v1::goal* goal, size_t depth) {
  std::string padding(depth, '\t');

  // Proposition
  const auto* simple_goal = dynamic_cast<const VAL_v1::simple_goal*>(goal);
  if (simple_goal != nullptr) {
    const VAL_v1::proposition* prop = simple_goal->getProp();
    os << padding << prop->head->getName() << *prop->args << " [" << prop << "]"
       << std::endl;
    return;
  }

  // Conjunction
  const auto* conj_goal = dynamic_cast<const VAL_v1::conj_goal*>(goal);
  if (conj_goal != nullptr) {
    os << padding << "and:" << std::endl;
    for (const VAL_v1::goal* g : *conj_goal->getGoals()) {
      PrintGoal(os, g, depth + 1);
    }
    return;
  }

  // Disjunction
  const auto* disj_goal = dynamic_cast<const VAL_v1::disj_goal*>(goal);
  if (disj_goal != nullptr) {
    os << padding << "or:" << std::endl;
    for (const VAL_v1::goal* g : *disj_goal->getGoals()) {
      PrintGoal(os, g, depth + 1);
    }
    return;
  }

  // Negation
  const auto* neg_goal = dynamic_cast<const VAL_v1::neg_goal*>(goal);
  if (neg_goal != nullptr) {
    os << padding << "neg:" << std::endl;
    PrintGoal(os, neg_goal->getGoal(), depth + 1);
    return;
  }

  // Quantification
  const auto* qfied_goal = dynamic_cast<const VAL_v1::qfied_goal*>(goal);
  if (qfied_goal != nullptr) {
    std::string quantifier;
    switch (qfied_goal->getQuantifier()) {
      case VAL_v1::quantifier::E_FORALL:
        quantifier = "forall";
        break;
      case VAL_v1::quantifier::E_EXISTS:
        quantifier = "exists";
        break;
    }
    os << padding << quantifier << *qfied_goal->getVars() << ":" << std::endl;
    PrintGoal(os, qfied_goal->getGoal(), depth + 1);
    return;
  }

  const auto* con_goal = dynamic_cast<const VAL_v1::con_goal*>(goal);
  const auto* constraint_goal =
      dynamic_cast<const VAL_v1::constraint_goal*>(goal);
  const auto* preference = dynamic_cast<const VAL_v1::preference*>(goal);
  const auto* imply_goal = dynamic_cast<const VAL_v1::imply_goal*>(goal);
  const auto* timed_goal = dynamic_cast<const VAL_v1::timed_goal*>(goal);
  const auto* comparison = dynamic_cast<const VAL_v1::comparison*>(goal);
  os << "con_goal: " << con_goal << std::endl;
  os << "constraint_goal: " << constraint_goal << std::endl;
  os << "preference: " << preference << std::endl;
  os << "disj_goal: " << disj_goal << std::endl;
  os << "imply_goal: " << imply_goal << std::endl;
  os << "timed_goal: " << timed_goal << std::endl;
  os << "comparison: " << comparison << std::endl;

  throw std::runtime_error("PrintGoal(): Goal type not implemented.");
}

void PrintEffects(std::ostream& os, const VAL_v1::effect_lists* effects,
                  size_t depth) {
  std::string padding(depth, '\t');
  for (const VAL_v1::simple_effect* effect : effects->add_effects) {
    os << padding << "(+) " << *effect << std::endl;
  }
  for (const VAL_v1::simple_effect* effect : effects->del_effects) {
    os << padding << "(-) " << *effect << std::endl;
  }
  for (const VAL_v1::forall_effect* effect : effects->forall_effects) {
    os << padding << "forall" << *effect->getVarsList() << ":" << std::endl;
    PrintEffects(os, effect->getEffects(), depth + 1);
  }
  for (const VAL_v1::cond_effect* effect : effects->cond_effects) {
    os << padding << "when:" << std::endl;
    PrintGoal(os, effect->getCondition(), depth + 1);
    os << padding << "then:" << std::endl;
    PrintEffects(os, effect->getEffects(), depth + 1);
  }
}

template <typename T>
void PrintArgs(std::ostream& os, const VAL_v1::typed_symbol_list<T>& args) {
  std::string separator;
  os << "(";
  for (const VAL_v1::parameter_symbol* param : args) {
    os << separator << param->getName() << " [" << param
       << "]: " << param->type->getName();
    if (separator.empty()) separator = ", ";
  }
  os << ")";
}

}  // namespace

namespace VAL_v1 {

std::ostream& operator<<(std::ostream& os, const domain& domain) {
  os << "DOMAIN" << std::endl;
  os << "======" << std::endl;
  os << "Name: " << domain.name << std::endl;

  os << "Requirements: " << pddl_req_flags_string(domain.req) << std::endl;

  os << "Types: " << std::endl;
  if (domain.types != nullptr) {
    for (const pddl_type* type : *domain.types) {
      os << "\t" << type->getName() << ": " << type->type->getName() << " ["
         << type << "]" << std::endl;
    }
  }

  os << "Constants: " << std::endl;
  if (domain.constants != nullptr) {
    for (const const_symbol* c : *domain.constants) {
      os << "\t" << c->getName() << " [" << c << "]"
         << ": " << c->type->getName() << std::endl;
    }
  }

  os << "Predicates:" << std::endl;
  if (domain.predicates != nullptr) {
    for (const pred_decl* pred : *domain.predicates) {
      os << "\t" << pred->getPred()->getName() << *pred->getArgs() << " ["
         << pred << "]" << std::endl;
    }
  }

  os << "Actions: " << std::endl;
  if (domain.ops != nullptr) {
    for (const operator_* op : *domain.ops) {
      os << "\t" << op->name->getName() << *op->parameters << std::endl;

      os << "\t\tPreconditions:" << std::endl;
      PrintGoal(os, op->precondition, 3);

      os << "\t\tEffects:" << std::endl;
      PrintEffects(os, op->effects, 3);
    }
  }

  return os;
}

std::ostream& operator<<(std::ostream& os, const problem& problem) {
  os << "PROBLEM" << std::endl;
  os << "=======" << std::endl;
  os << "Name: " << problem.name << std::endl;

  os << "Domain: " << problem.domain_name << std::endl;

  os << "Requirements: " << pddl_req_flags_string(problem.req) << std::endl;

  os << "Objects:" << std::endl;
  for (const const_symbol* object : *problem.objects) {
    os << "\t" << object->getName() << " [" << object << "]"
       << ": " << object->type->getName() << std::endl;
  }

  os << "Initial State:" << std::endl;
  PrintEffects(os, problem.initial_state, 1);

  os << "Goal:" << std::endl;
  PrintGoal(os, problem.the_goal, 1);

  return os;
}

std::ostream& operator<<(std::ostream& os, const simple_effect& effect) {
  os << effect.prop->head->getName() << *effect.prop->args << " ["
     << effect.prop->head << "]";
  return os;
}

std::ostream& operator<<(std::ostream& os, const var_symbol_list& args) {
  PrintArgs(os, args);
  return os;
}

std::ostream& operator<<(std::ostream& os, const parameter_symbol_list& args) {
  PrintArgs(os, args);
  return os;
}

}  // namespace VAL_v1
