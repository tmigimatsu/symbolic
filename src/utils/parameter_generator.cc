/**
 * parameter_generator.cc
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#include "symbolic/utils/parameter_generator.h"

#include <exception>  // std::runtime_error
#include <iostream>   // std::cerr

#include "symbolic/pddl.h"

namespace {

using ::symbolic::Object;
using ::symbolic::ParameterGenerator;

std::vector<std::vector<Object>> ParamTypes(
    const ParameterGenerator::ObjectTypeMap& object_map,
    const std::vector<Object>& params) {
  std::vector<std::vector<Object>> types;
  types.reserve(params.size());
  for (const Object& param : params) {
    try {
      types.push_back(object_map.at(param.type().name()));
    } catch (...) {
      std::cerr << "ParameterGenerator(): parameter type '"
                << param.type().name() << " not found in object map."
                << std::endl;
      types.clear();
      break;
      // throw std::runtime_error("ParameterGenerator(): parameter type '" +
      //                          param.type().name() +
      //                          "' not found in object map.");
    }
  }
  return types;
}

std::vector<const std::vector<Object>*> Options(
    const std::vector<std::vector<Object>>& param_types) {
  std::vector<const std::vector<Object>*> options;
  options.reserve(param_types.size());
  for (const std::vector<Object>& option : param_types) {
    options.push_back(&option);
  }
  return options;
}

}  // namespace

namespace symbolic {

ParameterGenerator::ParameterGenerator(const Pddl& pddl,
                                       const std::vector<Object>& params)
    : param_types_(ParamTypes(pddl.object_map(), params)) {
  Base::operator=(Base(Options(param_types_)));
}

// NOLINTNEXTLINE(bugprone-copy-constructor-init)
ParameterGenerator::ParameterGenerator(const ParameterGenerator& other)
    : param_types_(other.param_types_) {
  Base::operator=(Base(Options(param_types_)));
}

ParameterGenerator::ParameterGenerator(ParameterGenerator&& other) noexcept
    : param_types_(std::move(other.param_types_)) {
  Base::operator=(Base(Options(param_types_)));
}

ParameterGenerator& ParameterGenerator::operator=(
    const ParameterGenerator& rhs) {
  if (this == &rhs) return *this;

  pddl_ = rhs.pddl_;
  param_types_ = rhs.param_types_;
  Base::operator=(Base(Options(param_types_)));
  return *this;
}

ParameterGenerator& ParameterGenerator::operator=(
    ParameterGenerator&& rhs) noexcept {
  pddl_ = rhs.pddl_;
  param_types_ = std::move(rhs.param_types_);
  Base::operator=(Base(Options(param_types_)));
  return *this;
}

}  // namespace symbolic
