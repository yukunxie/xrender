## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

IF(EMBREE_GEOMETRY_POINT)

INCLUDE(tutorial)
ADD_TUTORIAL(point_geometry)
ADD_TUTORIAL_ISPC(point_geometry)
ADD_TUTORIAL_SYCL(point_geometry)

IF (BUILD_TESTING AND EMBREE_TESTING_INTENSITY GREATER 0)
  ADD_EMBREE_TUTORIAL_TEST(point_geometry "")
ENDIF()

ENDIF()
