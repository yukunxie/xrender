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
#include "Graphics/PhysicalImage.h"
#include "Graphics/RxSampler.h"
#include "Texture.h"
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
	// Front face
	{ -1.0f, -1.0f, 1.0f },
	{ 1.0f, -1.0f, 1.0f },
	{ 1.0f, 1.0f, 1.0f },
	{ -1.0f, 1.0f, 1.0f },
	// Back face
	{ -1.0f, -1.0f, -1.0f },
	{ -1.0f, 1.0f, -1.0f },
	{ 1.0f, 1.0f, -1.0f },
	{ 1.0f, -1.0f, -1.0f },
	// Top face
	{ -1.0f, 1.0f, -1.0f },
	{ -1.0f, 1.0f, 1.0f },
	{ 1.0f, 1.0f, 1.0f },
	{ 1.0f, 1.0f, -1.0f },
	// Bottom face
	{ -1.0f, -1.0f, -1.0f },
	{ 1.0f, -1.0f, -1.0f },
	{ 1.0f, -1.0f, 1.0f },
	{ -1.0f, -1.0f, 1.0f },
	// Right face
	{ 1.0f, -1.0f, -1.0f },
	{ 1.0f, 1.0f, -1.0f },
	{ 1.0f, 1.0f, 1.0f },
	{ 1.0f, -1.0f, 1.0f },
	// Left face
	{ -1.0f, -1.0f, -1.0f },
	{ -1.0f, -1.0f, 1.0f },
	{ -1.0f, 1.0f, 1.0f },
	{ -1.0f, 1.0f, -1.0f },
};

std::vector<std::uint32_t> GUniformCubeIndices = {
		// front
		0, 1, 2, 0, 2, 3,
		// back
		4, 5, 6, 4, 6, 7,
		// top
		8, 9, 10, 8, 10, 11,
		// bottom
		12, 13, 14, 12, 14, 15,
		// right
		16, 17, 18, 16, 18, 19,
		// left
		20, 21, 22, 20, 22, 23,
	};

/* adds a cube to the scene */
static unsigned int AddPrefilterCube(RTCDevice device_i, RTCScene scene_i)
{
	/* create a triangulated cube with 12 triangles and 8 vertices */
	RTCGeometry mesh = rtcNewGeometry(device_i, RTC_GEOMETRY_TYPE_TRIANGLE);


	Vector3f* vert = (Vector3f*)rtcSetNewGeometryBuffer(mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vector3f), GUniformCubeVertices.size());
	memcpy(vert, GUniformCubeVertices.data(), sizeof(GUniformCubeVertices[0]) * GUniformCubeVertices.size());

	unsigned int* index = (unsigned int*)rtcSetNewGeometryBuffer(mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), GUniformCubeIndices.size() / 3);
	memcpy(index, GUniformCubeIndices.data(), sizeof(GUniformCubeIndices[0]) * GUniformCubeIndices.size());

	rtcCommitGeometry(mesh);

	unsigned int geomID = rtcAttachGeometry(scene_i, mesh);

	rtcReleaseGeometry(mesh);
	return geomID;
}

TexturePtr PrefilterEnvironmentTexture(PhysicalImage32F& evnImage)
{
	SCOPED_PROFILING_GUARD("PrefilterEnvironmentTexture");
	int width  = 1024;
	int height = 1024;

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
		{ "ny", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f) },
		{ "py", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
		{ "pz", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
		{ "nz", glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f) }
	};


	RxSampler* sampler = RxSampler::CreateSampler();

	auto* cube = new TextureCube(width, height, TextureFormat::RGBA32FLOAT, nullptr);

	static const glm::vec4 PredefinedColors[] = {
		glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), // Red
		glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), // Green
		glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), // Blue
		glm::vec4(0.5f, 0.0f, 1.0f, 1.0f), // Purple
		glm::vec4(1.0f, 0.5f, 0.0f, 1.0f), // Orange
		glm::vec4(0.0f, 0.5f, 1.0f, 1.0f)	 // Indigo
	};

	constexpr int NUM_CORE = 6;
#pragma omp parallel for num_threads(NUM_CORE)
	for (int face = 0; face < 6; face++)
	{
		const auto&	   config = cameras[face];
		embree::Camera camera;
		camera.from					  = embree::Vec3fa(config.From.x, config.From.y, config.From.z);
		camera.to					  = embree::Vec3fa(config.Center.x, config.Center.y, config.Center.z);
		camera.up					  = embree::Vec3fa(config.Up.x, config.Up.y, config.Up.z);
		camera.fov					  = 90;
		camera.handedness			  = embree::Camera::Handedness::RIGHT_HANDED;
		embree::ISPCCamera ispcCamera = camera.getISPCCamera(width, height);

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

				int	 primId = rayhits.hit.primID;

				//printf("face=%d, primId=%d\n", face, primId);
				int	 index0 = GUniformCubeIndices[primId * 3 + 0];
				int	 index1 = GUniformCubeIndices[primId * 3 + 1];
				int	 index2 = GUniformCubeIndices[primId * 3 + 2];
				vec3 p0		= GUniformCubeVertices[index0];
				vec3 p1		= GUniformCubeVertices[index1];
				vec3 p2		= GUniformCubeVertices[index2];

				float u = rayhits.hit.u;
				float v = rayhits.hit.v;
				//Vector3f pos = u * p0 + v * p1 + (1 - u - v) * p2;
				vec3	pos	  = p0 + rayhits.hit.u * (p1 - p0) + rayhits.hit.v * (p2 - p0);
				vec2	uv	  = _SampleSphericalMap(glm::normalize(pos));
				//Color4f color = texture2D(&evnImage, uv);

				Color4f color = evnImage.ReadPixel(int(uv.x * evnImage.GetWidth()), int (uv.y * evnImage.GetHeight()));
				cube->WritePixel(face, width - 1 - x, height - 1 - y, color);
				//cube->WritePixel(face, x, height - y - 1, vec4(float(x)/ width, float(y)/height, 0, 1));
				//cube->WritePixel(face, x, height - y - 1, PredefinedColors[face]);

			}
		}
	}

	cube->AutoGenerateMipmaps();
	cube->SaveToFile("D:\\env\\env");

	return TexturePtr((Texture*)cube);
}