#pragma once

#include <embree4/rtcore.h>
#include <limits>
#include <iostream>
#include <simd/varying.h>
#include <omp.h>

#include <math/vec2.h>
#include <math/vec2fa.h>

#include <math/vec3.h>
#include <math/vec3fa.h>

#include <bvh/bvh.h>
#include <geometry/trianglev.h>

#include "imgui_impl_glfw.h"
#include "GLFW/glfw3.h"

#include "Camera.h"

class RenderScene
{
public:
	virtual void keypressed(int key);
	virtual void keyboardFunc(GLFWwindow* window, int key, int scancode, int action, int mods);
	virtual void clickFunc(GLFWwindow* window, int button, int action, int mods);
	virtual void motionFunc(GLFWwindow* window, double x, double y);

protected:
	float		   speed;
	embree::Vec3f  moveDelta;
	unsigned	   width;
	unsigned	   height;
	embree::Camera camera;
	int			   mouseMode;
	double		   clickX;
	double		   clickY;
};