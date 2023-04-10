#include "Prefilter.h"

#include <embree4/rtcore.h>
#include <limits>
#include <iostream>
#include <thread>
#include <simd/varying.h>
#include <omp.h>

#include <math/vec2.h>
#include <math/vec2fa.h>

#include <math/vec3.h>
#include <math/vec3fa.h>

#include <bvh/bvh.h>
#include <geometry/trianglev.h>

#include <iostream>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "Camera.h"
#include "Tools.h"
#include "Light.h"
#include "MeshProxy.h"
#include "Graphics/RxImage.h"
#include "Graphics/RxSampler.h"
#include "Object/MeshComponent.h"
#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <math.h>


static const vec2 invAtan = vec2(0.1591f, 0.3183f);

static vec2 _SampleSphericalMap(vec3 v)
{
	vec2 uv = vec2(glm::atan(v.z, v.x), asin(v.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

static std::vector<Vector3f> GUniformCubeVertices = {
	// back face
	{ -1.0f, -1.0f, -1.0f }, // bottom-left
	{ 1.0f, 1.0f, -1.0f },	 // top-right
	{ 1.0f, -1.0f, -1.0f },	 // bottom-right
	{ 1.0f, 1.0f, -1.0f },	 // top-right
	{ -1.0f, -1.0f, -1.0f }, // bottom-left
	{ -1.0f, 1.0f, -1.0f },	 // top-left
	// front face
	{ -1.0f, -1.0f, 1.0f }, // bottom-left
	{ 1.0f, -1.0f, 1.0f },	// bottom-right
	{ 1.0f, 1.0f, 1.0f },	// top-right
	{ 1.0f, 1.0f, 1.0f },	// top-right
	{ -1.0f, 1.0f, 1.0f },	// top-left
	{ -1.0f, -1.0f, 1.0f }, // bottom-left
	// left face
	{ -1.0f, 1.0f, 1.0f },	 // top-right
	{ -1.0f, 1.0f, -1.0f },	 // top-left
	{ -1.0f, -1.0f, -1.0f }, // bottom-left
	{ -1.0f, -1.0f, -1.0f }, // bottom-left
	{ -1.0f, -1.0f, 1.0f },	 // bottom-right
	{ -1.0f, 1.0f, 1.0f },	 // top-right
	// right face
	{ 1.0f, 1.0f, 1.0f },	// top-left
	{ 1.0f, -1.0f, -1.0f }, // bottom-right
	{ 1.0f, 1.0f, -1.0f },	// top-right
	{ 1.0f, -1.0f, -1.0f }, // bottom-right
	{ 1.0f, 1.0f, 1.0f },	// top-left
	{ 1.0f, -1.0f, 1.0f },	// bottom-left
	// bottom face
	{ -1.0f, -1.0f, -1.0f }, // top-right
	{ 1.0f, -1.0f, -1.0f },	 // top-left
	{ 1.0f, -1.0f, 1.0f },	 // bottom-left
	{ 1.0f, -1.0f, 1.0f },	 // bottom-left
	{ -1.0f, -1.0f, 1.0f },	 // bottom-right
	{ -1.0f, -1.0f, -1.0f }, // top-right
	// top face
	{ -1.0f, 1.0f, -1.0f }, // top-left
	{ 1.0f, 1.0f, 1.0f },	// bottom-right
	{ 1.0f, 1.0f, -1.0f },	// top-right
	{ 1.0f, 1.0f, 1.0f },	// bottom-right
	{ -1.0f, 1.0f, -1.0f }, // top-left
	{ -1.0f, 1.0f, 1.0f }	// bottom-left
};

/* adds a cube to the scene */
static unsigned int AddPrefilterCube(RTCDevice device_i, RTCScene scene_i)
{
	/* create a triangulated cube with 12 triangles and 8 vertices */
	RTCGeometry mesh = rtcNewGeometry(device_i, RTC_GEOMETRY_TYPE_TRIANGLE);


	Vector3f* vert = (Vector3f*)rtcSetNewGeometryBuffer(mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vector3f), GUniformCubeVertices.size());
	memcpy(vert, GUniformCubeVertices.data(), sizeof(GUniformCubeVertices[0]) * GUniformCubeVertices.size());

	std::vector<std::uint32_t> cubeIndices;
	for (std::uint32_t i = 0; i < GUniformCubeVertices.size(); ++i)
	{
		cubeIndices.push_back(i);
	}

	unsigned int* index = (unsigned int*)rtcSetNewGeometryBuffer(mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), cubeIndices.size() / 3);
	memcpy(index, cubeIndices.data(), sizeof(cubeIndices[0]) * cubeIndices.size());

	rtcCommitGeometry(mesh);

	unsigned int geomID = rtcAttachGeometry(scene_i, mesh);

	rtcReleaseGeometry(mesh);
	return geomID;
}

void PrefilterEnvironmentTexture(PhysicalImage32F& evnImage)
{
	SCOPED_PROFILING_GUARD("PrefilterEnvironmentTexture");
	int width = 1024;
	// renderTarget->GetWidth();
	int height = 1024;
	// renderTarget->GetHeight();

	RTCDevice device = rtcNewDevice("tri_accel=bvh4.triangle4v");

	error_handler(nullptr, rtcGetDeviceError(device));
	/* set error handler */
	rtcSetDeviceErrorFunction(device, error_handler, nullptr);


	RTCScene scene = rtcNewScene(device);
	AddPrefilterCube(device, scene);
	rtcCommitScene(scene);


	struct CameraConfig
	{
		std::string Face;
		vec3		From;
		vec3		Center;
		vec3		Up;
	};
	/*glm::mat4	 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);*/
	// Right Hand
	const CameraConfig cameras[] = {
		{ "px", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
		{ "nx", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
		{ "py", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f) },
		{ "ny", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f) },
		{ "pz", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
		{ "nz", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f) }
	};


	RxSampler* sampler = RxSampler::CreateSampler();

	constexpr int NUM_CORE = 6;
//#pragma omp parallel for num_threads(NUM_CORE)
	for (int i = 0; i < 6; i++)
	{
		const auto&	   config = cameras[i];
		embree::Camera camera;
		camera.from					  = embree::Vec3fa(config.From.x, config.From.y, config.From.z);
		camera.to					  = embree::Vec3fa(config.Center.x, config.Center.y, config.Center.z);
		camera.up					  = embree::Vec3fa(config.Up.x, config.Up.y, config.Up.z);
		camera.fov					  = 90;
		camera.handedness			  = embree::Camera::Handedness::RIGHT_HANDED;
		embree::ISPCCamera ispcCamera = camera.getISPCCamera(width, height);

		int		channels_num = 4;
		PhysicalImage32F renderImage(width, height, channels_num);

		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; x++)
			{
				auto dir = embree::Vec3fa(embree::normalize(x * ispcCamera.xfm.l.vx + y * ispcCamera.xfm.l.vy + ispcCamera.xfm.l.vz));

				RTCRayHit rayhits;

				rayhits.ray.org_x  = ispcCamera.xfm.p.x;
				rayhits.ray.org_y  = ispcCamera.xfm.p.y;
				rayhits.ray.org_z  = ispcCamera.xfm.p.z;
				rayhits.ray.dir_x  = dir.x;
				rayhits.ray.dir_y  = dir.y;
				rayhits.ray.dir_z  = dir.z;
				rayhits.ray.tnear  = 0.f;
				rayhits.ray.tfar   = 2000;
				rayhits.ray.time   = 1;
				rayhits.ray.mask   = 0xFFFFFFFF;
				rayhits.hit.geomID = RTC_INVALID_GEOMETRY_ID;

				rtcIntersect1(scene, &rayhits);

				Assert(rayhits.hit.geomID != RTC_INVALID_GEOMETRY_ID);

				int		 primId = rayhits.hit.primID;
				Vector3f p0		= GUniformCubeVertices[primId * 3 + 0];
				Vector3f p1		= GUniformCubeVertices[primId * 3 + 1];
				Vector3f p2		= GUniformCubeVertices[primId * 3 + 2];

				vec3 pos = p0 + rayhits.hit.u * (p1 - p0) + rayhits.hit.v * (p2 - p0);

				//_SampleSphericalMap
				pos		= glm::normalize(pos);
				vec2 uv = vec2(glm::atan(pos.z, pos.x), glm::asin(pos.y));
				uv *= invAtan;
				uv += 0.5;

				Color4f color = texture2D(&evnImage, uv);
				/*Color4B output(int(color.x * 255), int(color.y * 255), int(color.z * 255), 255);*/
				renderImage.WritePixel(x, height - y - 1, color);
			}
		}
		std::string filename = "D:\\cube_" + config.Face + ".hdr";
		renderImage.SaveToFile(filename.c_str());

		auto mip = renderImage.DownSample();
		std::string mipFilename = "D:\\cube_" + config.Face + ".mip" + ".hdr";
		mip->SaveToFile(mipFilename.c_str());
		break;
	}
}