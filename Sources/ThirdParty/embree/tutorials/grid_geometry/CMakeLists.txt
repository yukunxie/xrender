## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

IF(EMBREE_GEOMETRY_GRID)

INCLUDE(tutorial)
ADD_TUTORIAL(grid_geometry)
ADD_TUTORIAL_ISPC(grid_geometry)
ADD_TUTORIAL_SYCL(grid_geometry)

IF (BUILD_TESTING AND EMBREE_TESTING_INTENSITY GREATER 0)
  ADD_EMBREE_TUTORIAL_TEST(grid_geometry "")
ENDIF()

ENDIF()
