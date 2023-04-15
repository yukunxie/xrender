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
#include "Graphics/Material.h"
#include "RTRender.h"

std::map<int, MeshComponent*> GMeshComponentProxies;

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
	// init global texturs;
	{
		EnvironmentTextures* envTextures = GetEnvironmentData();
		envTextures->BRDFTexture = std::make_shared<Texture2D>("Engine/lutBRDF.png");
		/*PhysicalImage32F image("Textures/hdr/newport_loft.hdr");
		mEnvTexture = PrefilterEnvironmentTexture(image);*/
		envTextures->EnvTexture = std::make_shared<TextureCube>("SkyBox0");

		envTextures->IrradianceTexture = std::make_shared<TextureCube>("PreIrradiance0");

		envTextures->SphericalEnvTexture = std::make_shared<Texture2D>("Textures/hdr/newport_loft.hdr");
		envTextures->SphericalEnvTexture->AutoGenerateMipmaps();
	}

	RTCDevice device = rtcNewDevice("tri_accel=bvh4.triangle4v");

	error_handler(nullptr, rtcGetDeviceError(device));
	/* set error handler */
	rtcSetDeviceErrorFunction(device, error_handler, nullptr);


	RTCScene scene = rtcNewScene(device);

	// Add Sky box
	{
		//auto	cubeMesh = MeshComponentBuilder::CreateBox("", Vector3f(1000.0f));
		auto	cubeMesh = MeshComponentBuilder::CreateSkyBox(Vector3f(1000.0f));
		Entity* skybox	 = new Entity();
		skybox->AddComponment(cubeMesh);
		std::vector<Entity*> entities = { skybox };
		//AddEntityToEmbreeScene(device, scene, entities);
	}

	{
		// auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Models/deer.gltf");
		// auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Models/color_teapot_spheres.gltf");
		 auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Scenes/Sponza/Sponza.gltf");
		// auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Scenes/cornellbox/cornellBox-2.80-Eevee-gltf.gltf");
		//auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Models/DamagedHelmet/glTF-Embedded/DamagedHelmet.gltf");
		AddEntityToEmbreeScene(device, scene, gltfSceneEntities);
	}

	{
		 auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Models/deer.gltf");
		// auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Models/color_teapot_spheres.gltf");
		//auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Scenes/Sponza/Sponza.gltf");
		// auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Scenes/cornellbox/cornellBox-2.80-Eevee-gltf.gltf");
		 //auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Models/DamagedHelmet/glTF-Embedded/DamagedHelmet.gltf");
		AddEntityToEmbreeScene(device, scene, gltfSceneEntities);
	}

	rtcCommitScene(scene);

	RTCRayHit rayhit;

	//Vector3f pos   = { -3, 5.0f, -3.0f };
	Vector3f pos   = { 50, 10.0f, 0.0f };
	Vector3f focus = { .0f, 0.0f, 0.0f };
	Vector3f up	   = { 0, 1, 0 };
	float	 fov   = 60;


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