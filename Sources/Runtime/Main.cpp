// PVSCompress.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

//#define TASKING_TBB 1
//#define __AVX2__ 1

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

//#include <stb_image.h>
//#include <stb_image_write.h>

#include "Camera.h"
#include "Tools.h"
#include "Light.h"
#include "MeshProxy.h"
#include "Graphics/PhysicalImage.h"
#include "Graphics/RxSampler.h"
#include "Graphics/Prefilter.h"
#include "Object/MeshComponent.h"

#include "Loader/GLTFLoader.h"
#include "XRenderer.h"
#include "Renderer/PBRRender.h"
#include "RTRender.h"

// std::map<int, RTCMeshProxy*> GMeshProxies;
std::map<int, MeshComponent*> GMeshComponentProxies;


/* adds a cube to the scene */
unsigned int addCube(RTCDevice device_i, RTCScene scene_i, const Vector3f& pos)
{
	/* create a triangulated cube with 12 triangles and 8 vertices */
	RTCGeometry mesh = rtcNewGeometry(device_i, RTC_GEOMETRY_TYPE_TRIANGLE);

	std::vector<Vector3f>	  cubeVertices;
	std::vector<Vector3f>	  cubeNormals;
	std::vector<Vector2f>	  cubeUVs;
	std::vector<unsigned int> cubeIndices;
	GenerateCube(cubeVertices,
				 cubeNormals,
				 cubeUVs,
				 cubeIndices);

	Vector3f* vert = (Vector3f*)rtcSetNewGeometryBuffer(mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vector3f), cubeVertices.size());
	memcpy(vert, cubeVertices.data(), sizeof(cubeVertices[0]) * cubeVertices.size());

	unsigned int* index = (unsigned int*)rtcSetNewGeometryBuffer(mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), cubeIndices.size() / 3);
	memcpy(index, cubeIndices.data(), sizeof(cubeIndices[0]) * cubeIndices.size());


	/*AffineSpace3fa transform = embree::one;
	transform				 = transform.translate(pos).scale({3.0f, 3.0f, 3.0f});
	rtcSetGeometryTransform(mesh, 0, RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR, &transform);*/

	rtcCommitGeometry(mesh);

	unsigned int geomID = rtcAttachGeometry(scene_i, mesh);
	// GMeshProxies[geomID] = new RTCMeshProxy(cubeUVs, cubeNormals, cubeIndices);
	std::cout << "AddCube geomID=" << geomID << std::endl;


	rtcReleaseGeometry(mesh);
	return geomID;
}

unsigned int addSphere(RTCDevice device_i, RTCScene scene_i, const Vector3f& pos)
{
	int						  radius	 = 2;
	int						  latitudes	 = 32;
	int						  longitudes = 32;
	std::vector<Vector3f>	  sphereVertices;
	std::vector<Vector3f>	  sphereNormals;
	std::vector<Vector2f>	  sphereUVs;
	std::vector<unsigned int> sphereIndices;
	GenerateSphereSmooth(radius,
						 latitudes,
						 longitudes,
						 sphereVertices,
						 sphereNormals,
						 sphereUVs,
						 sphereIndices);

	RTCGeometry mesh = rtcNewGeometry(device_i, RTC_GEOMETRY_TYPE_TRIANGLE);

	Vector3f* vert = (Vector3f*)rtcSetNewGeometryBuffer(mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vector3f), sphereVertices.size());
	memcpy(vert, sphereVertices.data(), sizeof(sphereVertices[0]) * sphereVertices.size());

	// Vector3f* norm = (Vector3f*)rtcSetNewGeometryBuffer(mesh, RTC_BUFFER_TYPE_NORMAL, 1, RTC_FORMAT_FLOAT3, sizeof(Vector3f), sphereNormals.size());
	// memcpy(norm, sphereNormals.data(), sizeof(Vector3f) * sphereNormals.size());

	//   Vector2f* uv = (Vector2f*)rtcSetNewGeometryBuffer(mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 2, RTC_FORMAT_FLOAT2, sizeof(Vector2f), sphereUVs.size());
	// memcpy(uv, sphereUVs.data(), sizeof(Vector2f) * sphereUVs.size());

	unsigned int* index = (unsigned int*)rtcSetNewGeometryBuffer(mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), sphereIndices.size() / 3);
	memcpy(index, sphereIndices.data(), sizeof(sphereIndices[0]) * sphereIndices.size());

	rtcCommitGeometry(mesh);

	unsigned int geomID = rtcAttachGeometry(scene_i, mesh);

	std::cout << "AddSphere geomID=" << geomID << std::endl;
	// GMeshProxies[geomID] = new RTCMeshProxy(sphereUVs, sphereNormals, sphereIndices);

	// rtcReleaseGeometry(mesh);
	return geomID;
}


void AddEntityToEmbreeScene(RTCDevice device_i, RTCScene scene_i, const std::vector<Entity*>& entities)
{
	for (auto entity : entities)
	{
		entity->AddToEmbreeScene(device_i, scene_i);
	}
}

int width  = 1024;
int height = 1024;

int main()
{
	

	RTCDevice device = rtcNewDevice("tri_accel=bvh4.triangle4v");

	error_handler(nullptr, rtcGetDeviceError(device));
	/* set error handler */
	rtcSetDeviceErrorFunction(device, error_handler, nullptr);


	RTCScene scene = rtcNewScene(device);

	// Add Sky box
	{
		auto	cubeMesh = MeshComponentBuilder::CreateBox("", Vector3f(1000.0f));
		Entity* skybox	 = new Entity();
		skybox->AddComponment(cubeMesh);
		std::vector<Entity*> entities = { skybox };
		AddEntityToEmbreeScene(device, scene, entities);
	}

	{
		// auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Models/deer.gltf");
		// auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Models/color_teapot_spheres.gltf");
		// auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Scenes/Sponza/Sponza.gltf");
		// auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Scenes/cornellbox/cornellBox-2.80-Eevee-gltf.gltf");
		auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Models/DamagedHelmet/glTF-Embedded/DamagedHelmet.gltf");
		AddEntityToEmbreeScene(device, scene, gltfSceneEntities);
	}
	

	/*PhysicalImage32F image("Textures/hdr/newport_loft.hdr");
	PrefilterEnvironmentTexture(image);*/


	// addHair(device, scene);

	// addCube(device, scene, Vec3fa(-1, 0, 0));
	// addCube(device, scene, Vec3fa(1, 0, 0));
	// addCube(device, scene, Vec3fa(0, 0, -1));
	// addCube(device, scene, Vec3fa(0, 0, 1));
	// addCube(device, scene, Vec3fa(0, 5, 5));
	// addSphere(device, scene, Vec3fa(0, 0, 0));

	rtcCommitScene(scene);

	RTCRayHit rayhit;

	Vector3f pos   = { 0, -20.0f, 0.0f };
	Vector3f focus = { 1.0f, 0.0f, 0.0f };
	Vector3f up	   = { 0, 1, 0 };
	float	 fov   = 120;


	RTCRayQueryContext context;
	void*			   userRayExt = nullptr; //!< can be used to pass extended ray data to callbacks
	void*			   tutorialData;

	rtcInitRayQueryContext(&context);
	// context.tutorialData = (void*)&data;

	RTCIntersectArguments args;
	rtcInitIntersectArguments(&args);
	args.context	  = &context;
	args.flags		  = RTC_RAY_QUERY_FLAG_INCOHERENT;
	args.feature_mask = RTC_FEATURE_FLAG_NONE;
#if USE_ARGUMENT_CALLBACKS && ENABLE_FILTER_FUNCTION
	args.filter = nullptr;
#endif

	// print_bvh(scene);

	int		channels_num = 4;
	PhysicalImage renderImage(width, height, channels_num);

	auto sampler = RxSampler::CreateSampler();

	GlobalConstantBuffer cGlobalBuffer;
	{
		cGlobalBuffer.EyePos		= Vector4f(pos.x, pos.y, pos.z, 1.0f);
		cGlobalBuffer.SunLight		= Vector4f(-10.0f, -10.0f, -10.0f, 0.0f);
		cGlobalBuffer.SunLightColor = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
		cGlobalBuffer.ViewMatrix;
		cGlobalBuffer.ProjMatrix;
	}

	BatchBuffer cBatchBuffer;
	{
		cBatchBuffer.WorldMatrix = TMat4x4(1.0f);
	}

	PBRRender pbrRender;

	std::thread rtRenderThread([&pbrRender, &renderImage, &pos, &focus, &up, &fov, &scene]()
							   { RTRender(pos, focus, up, fov, &renderImage, scene, pbrRender); });


	const auto mouse_callback = [&](float xoffset, float yoffset)
	{
		auto mat = glm::lookAtRH(pos, focus, up);

		//float sensitivity = 0.05f; //灵敏度
		//xoffset *= sensitivity;
		//yoffset *= sensitivity;

		//yaw += xoffset;
		//pitch += yoffset;

		glm::vec3 cameraFront = glm::normalize(focus - pos);
		float	  raw		  = glm::degrees(glm::asin(cameraFront.y));
		glm::vec3 cameraDirection = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
		float	  yaw			  = glm::degrees(glm::atan(cameraDirection.z, cameraDirection.x));
		glm::vec3 cameraUp		  = glm::normalize(up);
		float	  pitch			  = glm::degrees(glm::acos(glm::dot(cameraDirection, cameraUp)));

		yaw += xoffset;
		pitch += yoffset;

		pitch = pitch > 89.0f ? 89.0f : pitch;
		pitch = pitch < -89.0f ? -89.0f : pitch;

		glm::vec3 front;
		//根据俯仰和偏航角度来算出此向量，也就是速度在三个维度的数值
		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch)) - 1;

		focus = glm::normalize(front) * glm::length(focus - pos);
		
		/*std::thread rtRenderThread([&pbrRender, &renderImage, &pos, &focus, &up, &fov, &scene]()
								   { RTRender(pos, focus, up, fov, &renderImage, scene, pbrRender); });*/

		//RTRender(pos, focus, up, fov, &renderImage, scene, pbrRender);
	};

	Renderer(&renderImage, mouse_callback);
	renderImage.SaveToFile("D:/x_render.png");

	rtRenderThread.join();

	rtcReleaseScene(scene);
	rtcReleaseDevice(device);
}