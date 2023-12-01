//@HEADER
// ************************************************************************
//
//                        Kokkos v. 4.0
//       Copyright (2022) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Part of Kokkos, under the Apache License v2.0 with LLVM Exceptions.
// See https://kokkos.org/LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//@HEADER
#include <Kokkos_Core.hpp>
#include <gtest/gtest.h>

namespace testing::internal {
// accessing gtest internals is not very clean, but gtest provides no public access...
extern bool g_help_flag;
} // namespace testing::internal

int main(int argc, char* argv[]) {
  
  ::testing::InitGoogleTest(&argc, argv);
  int result = 0;
  if (::testing::GTEST_FLAG(list_tests) || ::testing::internal::g_help_flag) {
    result = RUN_ALL_TESTS();
  } else {
    Kokkos::initialize(argc, argv);
    result = RUN_ALL_TESTS();
    Kokkos::finalize();
  }

  return result;
}