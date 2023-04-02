#include <embree4/rtcore.h>
#include <limits>
#include <iostream>
#include <simd/varying.h>

#include <math/vec2.h>
#include <math/vec2fa.h>

#include <math/vec3.h>
#include <math/vec3fa.h>

#include <bvh/bvh.h>
#include <geometry/trianglev.h>

/* error reporting function */
void error_handler(void* userPtr, const RTCError code, const char* str = nullptr);

void print_bvh4_triangle4v(embree::BVH4::NodeRef node, size_t depth);

/* prints the triangle BVH of a scene */
void print_bvh(RTCScene scene);
