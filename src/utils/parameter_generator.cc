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

namespace {

using ::symbolic::ParameterGenerator;
using ::symbolic::Object;

std::vector<const std::vector<Object>*>
ParamTypes(const ParameterGenerator::ObjectTypeMap& object_map,
           const std::vector<Object>& params) {
  std::vector<const std::vector<Object>*> types;
  types.reserve(params.size());
  for (const Object& param : params) {
    try {
      types.push_back(&object_map.at(param.type().name()));
    } catch (...) {
      throw std::runtime_error("ParameterGenerator(): parameter type '" + param.type().name() +
                               "' not found in object map.");
    }
  }
  return types;
}

}  // namespace

namespace symbolic {

ParameterGenerator::ParameterGenerator(const ObjectTypeMap& object_map,
                                       const std::vector<Object>& params)
    : CombinationGenerator(ParamTypes(object_map, params)) {}

}  // namespace symbolic
