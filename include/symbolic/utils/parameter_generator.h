/**
 * parameter_generator.h
 *
 * Copyright 2018. All Rights Reserved.
 *
 * Created: November 28, 2018
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_UTILS_PARAMETER_GENERATOR_H_
#define SYMBOLIC_UTILS_PARAMETER_GENERATOR_H_

#include <vector>  // std::vector

#include "symbolic/object.h"
#include "symbolic/utils/combination_generator.h"

namespace symbolic {

// CombinatorGenerator class contains no mutable member variables. All state is
// held inside the iterator, meaning multiple parallel instances can use the
// same generator.
class ParameterGenerator : public CombinationGenerator<const std::vector<Object>> {

 public:

  using ObjectTypeMap = std::map<std::string, std::vector<Object>>;

  ParameterGenerator(const ObjectTypeMap& object_map, const std::vector<Object>& params);

};

}  // namespace symbolic

#endif  // SYMBOLIC_UTILS_PARAMETER_GENERATOR_H_
