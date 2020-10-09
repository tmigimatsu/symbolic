/**
 * main.cc
 *
 * Copyright 2020. All Rights Reserved.
 *
 * Created: April 1, 2020
 * Authors: Toki Migimatsu
 */

#define DOCTEST_CONFIG_IMPLEMENTATION_IN_DLL
#include <doctest/doctest.h>

int main(int argc, char** argv) {
  doctest::Context context(argc, argv);
  return context.run();
}
