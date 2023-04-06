#include "RenderScene.h"

/* called when a key is pressed */
void RenderScene::keypressed(int key)
{
}

void RenderScene::keyboardFunc(GLFWwindow* window_in, int key, int scancode, int action, int mods)
{
	//ImGui_ImplGlfw_KeyCallback(window_in, key, scancode, action, mods);
	//if (ImGui::GetIO().WantCaptureKeyboard)
	//	return;

	//if (action == GLFW_PRESS)
	//{
	//	/* call tutorial keyboard handler */
	//	keypressed(key);

	//	if (mods & GLFW_MOD_CONTROL)
	//	{
	//		switch (key)
	//		{
	//		case GLFW_KEY_UP:
	//			debug_int0++;
	//			set_parameter(1000000, debug_int0);
	//			PRINT(debug_int0);
	//			break;
	//		case GLFW_KEY_DOWN:
	//			debug_int0--;
	//			set_parameter(1000000, debug_int0);
	//			PRINT(debug_int0);
	//			break;
	//		case GLFW_KEY_LEFT:
	//			debug_int1--;
	//			set_parameter(1000001, debug_int1);
	//			PRINT(debug_int1);
	//			break;
	//		case GLFW_KEY_RIGHT:
	//			debug_int1++;
	//			set_parameter(1000001, debug_int1);
	//			PRINT(debug_int1);
	//			break;
	//		}
	//	}
	//	else
	//	{
	//		switch (key)
	//		{
	//		case GLFW_KEY_LEFT:
	//			camera.rotate(-0.02f, 0.0f);
	//			break;
	//		case GLFW_KEY_RIGHT:
	//			camera.rotate(+0.02f, 0.0f);
	//			break;
	//		case GLFW_KEY_UP:
	//			camera.move(0.0f, 0.0f, +speed);
	//			break;
	//		case GLFW_KEY_DOWN:
	//			camera.move(0.0f, 0.0f, -speed);
	//			break;
	//		case GLFW_KEY_PAGE_UP:
	//			speed *= 1.2f;
	//			break;
	//		case GLFW_KEY_PAGE_DOWN:
	//			speed /= 1.2f;
	//			break;

	//		case GLFW_KEY_W:
	//			moveDelta.z = +1.0f;
	//			break;
	//		case GLFW_KEY_S:
	//			moveDelta.z = -1.0f;
	//			break;
	//		case GLFW_KEY_A:
	//			moveDelta.x = -1.0f;
	//			break;
	//		case GLFW_KEY_D:
	//			moveDelta.x = +1.0f;
	//			break;

	//		case GLFW_KEY_F:
	//			glfwDestroyWindow(window);
	//			if (fullscreen)
	//			{
	//				width  = window_width;
	//				height = window_height;
	//				window = createStandardWindow(width, height);
	//				setCallbackFunctions(window);
	//			}
	//			else
	//			{
	//				window_width  = width;
	//				window_height = height;
	//				window		  = createFullScreenWindow();
	//				setCallbackFunctions(window);
	//			}
	//			glfwMakeContextCurrent(window);
	//			fullscreen = !fullscreen;
	//			break;

	//		case GLFW_KEY_C:
	//			std::cout << camera.str() << std::endl;
	//			break;
	//		case GLFW_KEY_HOME:
	//			g_debug = clamp(g_debug + 0.01f);
	//			PRINT(g_debug);
	//			break;
	//		case GLFW_KEY_END:
	//			g_debug = clamp(g_debug - 0.01f);
	//			PRINT(g_debug);
	//			break;

	//		case GLFW_KEY_SPACE:
	//		{
	//			Ref<Image> image = new Image4uc(width, height, (Col4uc*)pixels, true, "", true);
	//			storeImage(image, "screenshot.tga");
	//			break;
	//		}

	//		case GLFW_KEY_ESCAPE:
	//		case GLFW_KEY_Q:
	//			glfwSetWindowShouldClose(window, 1);
	//			break;
	//		}
	//	}
	//}
	//else if (action == GLFW_RELEASE)
	//{
	//	switch (key)
	//	{
	//	case GLFW_KEY_W:
	//		moveDelta.z = 0.0f;
	//		break;
	//	case GLFW_KEY_S:
	//		moveDelta.z = 0.0f;
	//		break;
	//	case GLFW_KEY_A:
	//		moveDelta.x = 0.0f;
	//		break;
	//	case GLFW_KEY_D:
	//		moveDelta.x = 0.0f;
	//		break;
	//	}
	//}
}

bool device_pick(const float	   x,
				 const float	   y,
				 const embree::ISPCCamera& camera,
				 embree::Vec3fa&		   hitPos)
{
	/* initialize ray */
	embree::RayHit ray;
	ray.org		= embree::Vec3ff(camera.xfm.p);
	ray.dir		= embree::Vec3ff(normalize(x * camera.xfm.l.vx + y * camera.xfm.l.vy + camera.xfm.l.vz));
	ray.tnear() = 0.0f;
	ray.tfar	= embree::inf;
	ray.geomID	= RTC_INVALID_GEOMETRY_ID;
	ray.primID	= RTC_INVALID_GEOMETRY_ID;
	ray.mask	= -1;
	ray.time()	= 0.0f;

	///* intersect ray with scene */
	//rtcIntersect1(g_scene, RTCRayHit1_(ray));

	/* shade pixel */
	if (ray.geomID == RTC_INVALID_GEOMETRY_ID)
	{
		hitPos = embree::Vec3fa(0.0f, 0.0f, 0.0f);
		return false;
	}
	else
	{
		hitPos = ray.org + ray.tfar * ray.dir;
		return true;
	}
}

void RenderScene::clickFunc(GLFWwindow* window, int button, int action, int mods)
{
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
	if (ImGui::GetIO().WantCaptureMouse)
		return;

	double x, y;
	glfwGetCursorPos(window, &x, &y);

	if (action == GLFW_RELEASE)
	{
		mouseMode = 0;
	}
	else if (action == GLFW_PRESS)
	{
		if (button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			embree::ISPCCamera ispccamera = camera.getISPCCamera(width, height);
			embree::Vec3fa	   p;
			bool			   hit = device_pick(float(x), float(y), ispccamera, p);

			if (hit)
			{
				embree::Vec3fa delta = p - camera.to;
				embree::Vec3fa right = normalize(ispccamera.xfm.l.vx);
				embree::Vec3fa up	 = normalize(ispccamera.xfm.l.vy);
				camera.to	 = p;
				camera.from += dot(delta, right) * right + dot(delta, up) * up;
			}
		}
		else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
		{
			printf("pixel pos (%d, %d)\n", (int)x, (int)y);
		}
		else
		{
			clickX = x;
			clickY = y;
			if (button == GLFW_MOUSE_BUTTON_LEFT && mods == GLFW_MOD_SHIFT)
				mouseMode = 1;
			else if (button == GLFW_MOUSE_BUTTON_LEFT && mods == GLFW_MOD_CONTROL)
				mouseMode = 3;
			else if (button == GLFW_MOUSE_BUTTON_LEFT)
				mouseMode = 4;
		}
	}
}

void RenderScene::motionFunc(GLFWwindow* window, double x, double y)
{
	ImGui_ImplGlfw_CursorPosCallback(window, x, y);
	if (ImGui::GetIO().WantCaptureMouse)
		return;

	float dClickX = float(clickX - x), dClickY = float(clickY - y);
	clickX = x;
	clickY = y;

	switch (mouseMode)
	{
	case 1:
		camera.rotateOrbit(-0.005f * dClickX, 0.005f * dClickY);
		break;
	case 2:
		break;
	case 3:
		camera.dolly(-dClickY);
		break;
	case 4:
		camera.rotate(-0.005f * dClickX, 0.005f * dClickY);
		break;
	}
}