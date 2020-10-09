/**
 * doctest.h
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: October 9, 2020
 * Authors: Toki Migimatsu
 */

#ifndef SYMBOLIC_UTILS_DOCTEST_H_
#define SYMBOLIC_UTILS_DOCTEST_H_

#ifndef DOCTEST_CONFIG_DISABLE
#define DOCTEST_CONFIG_IMPLEMENTATION_IN_DLL
#define DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#endif  // DOCTEST_CONFIG_DISABLE

#include <doctest/doctest.h>

#include "symbolic/pddl.h"

namespace symbolic {
namespace testing {

struct Fixture {
  Pddl pddl = Pddl("../resources/domain.pddl", "../resources/problem.pddl");
};

}  // namespace testing
}  // namespace symbolic

#endif  // SYMBOLIC_UTILS_DOCTEST_H_
