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

#include <unordered_map>  // std::unordered_map
#include <vector>         // std::vector

#include "symbolic/object.h"
#include "symbolic/utils/combination_generator.h"

namespace symbolic_v1 {

class ParameterGenerator
    : public CombinationGenerator<const std::vector<Object>> {
 public:
  using Base = CombinationGenerator<const std::vector<Object>>;
  using ObjectTypeMap = std::unordered_map<std::string, std::vector<Object>>;

  ParameterGenerator() = default;
  ~ParameterGenerator() override = default;

  ParameterGenerator(const ObjectTypeMap& object_map,
                     const std::vector<Object>& params);

  ParameterGenerator(const ParameterGenerator& other);
  ParameterGenerator(ParameterGenerator&& other) noexcept;
  ParameterGenerator& operator=(const ParameterGenerator& rhs);
  ParameterGenerator& operator=(ParameterGenerator&& rhs) noexcept;


  // ParameterGenerator(ObjectTypeMap&& object_map,
  //                    const std::vector<Object>& params);

 private:
  // Store parameter types for portability
  std::vector<std::vector<Object>> param_types_;
};

}  // namespace symbolic_v1

#endif  // SYMBOLIC_UTILS_PARAMETER_GENERATOR_H_
