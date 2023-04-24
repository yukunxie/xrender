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
#include "Graphics/Material.h"
#include "Raytracer.h"
#include "Renderer/PBRRender.h"

std::map<int, RTInstanceData> GMeshComponentProxies;

void AddEntityToEmbreeScene(RTCDevice device_i, RTCScene scene_i, const std::vector<Entity*>& entities)
{
	for (auto entity : entities)
	{
		entity->AddToEmbreeScene(device_i, scene_i);
	}
}

int width  = 4096;
int height = 4096;

int main()
{
	// init global texturs;
	{
		EnvironmentTextures* envTextures = GetEnvironmentData();
		envTextures->BRDFTexture		 = std::make_shared<Texture2D>("Engine/lutBRDF.png");
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
		// auto	cubeMesh = MeshComponentBuilder::CreateBox("", Vector3f(1000.0f));
		auto	cubeMesh = MeshComponentBuilder::CreateSkyBox(Vector3f(10000.0f));
		Entity* skybox	 = new Entity();
		skybox->AddComponment(cubeMesh);
		std::vector<Entity*> entities = { skybox };
		AddEntityToEmbreeScene(device, scene, entities);
	}



#if 1
	{
		// auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Models/deer.gltf");
		// auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Models/color_teapot_spheres.gltf");
		auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Scenes/Sponza/Sponza.gltf");
		// auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Scenes/cornellbox/cornellBox-2.80-Eevee-gltf.gltf");
		// auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Models/DamagedHelmet/glTF-Embedded/DamagedHelmet.gltf");
		AddEntityToEmbreeScene(device, scene, gltfSceneEntities);
	}
#else
	{
		auto	cubeMesh = MeshComponentBuilder::CreateBox("");
		Entity* plane	 = new Entity();
		plane->AddComponment(cubeMesh);
		std::vector<Entity*> entities = { plane };
		entities[0]->SetScale(vec3(1000, 1.0f, 1000));
		entities[0]->SetPosition(vec3(0, -1.0f, 0));
		MeshComponent* mesh = entities[0]->GetComponent<MeshComponent>();
		float		   roughness = 0.9f;
		float		   metallic	 = 0.1f;
		mesh->GetMaterial()->SetFloat("roughnessFactor", 0, 1, &roughness);
		mesh->GetMaterial()->SetFloat("metallicFactor", 0, 1, &metallic);
		AddEntityToEmbreeScene(device, scene, entities);
	}
#endif

	{
		 //auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Models/deer.gltf");
		 //auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Models/cerberus/cerberus.gltf");
		// auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Scenes/Sponza/Sponza.gltf");
		// auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Scenes/cornellbox/cornellBox-2.80-Eevee-gltf.gltf");
		auto gltfSceneEntities = GLTFLoader::LoadModelFromGLTF("Models/DamagedHelmet/glTF-Embedded/DamagedHelmet.gltf");
		 gltfSceneEntities[0]->SetScale(vec3(0.5f));
		 gltfSceneEntities[0]->SetPosition(vec3(-4, 2, 0));

		AddEntityToEmbreeScene(device, scene, gltfSceneEntities);
	}


	rtcCommitScene(scene);

	RTCRayHit rayhit;

	Vector3f pos   = { 10, 2.0f, 0.0f };
	//Vector3f pos   = { 50, 10.0f, 0.0f };
	Vector3f focus = { .0f, 2.0f, 0.0f };
	Vector3f up	   = { 0, 1, 0 };
	float	 fov   = 60;

	glm::vec3 cameraFront = glm::normalize(focus - pos);

	//pos = pos + cameraFront * 30.0f;
	//focus = pos + cameraFront * 10.0f;


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

	int			  channels_num = 4;
	PhysicalImage renderImage(width, height, channels_num);

	auto sampler = RxSampler::CreateSampler();


	GlobalConstantBuffer cGlobalBuffer;
	{
		cGlobalBuffer.EyePos		= Vector4f(pos.x, pos.y, pos.z, 1.0f);
		cGlobalBuffer.SunLight		= Vector4f(.0f, -1.0f, 0.0f, 0.0f);
		cGlobalBuffer.SunLightColor = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
		cGlobalBuffer.ViewMatrix	= glm::lookAtRH(pos, focus, up);
		cGlobalBuffer.ProjMatrix	= glm::perspectiveFovRH(fov, float(width), float(height), 0.1f, 20000.0f);


		glm::vec3 center(0.0f, 0.0f, 0.0f); // 立方体中心位置
		float	  length = 10.0f;			// 立方体边长的一半

		// 定义8个顶点坐标
		glm::vec3 vertex0(center.x - length, center.y - length, center.z - length);
		glm::vec3 vertex1(center.x + length, center.y - length, center.z - length);
		glm::vec3 vertex2(center.x - length, center.y + length, center.z - length);
		glm::vec3 vertex3(center.x + length, center.y + length, center.z - length);
		glm::vec3 vertex4(center.x - length, center.y - length, center.z + length);
		glm::vec3 vertex5(center.x + length, center.y - length, center.z + length);
		glm::vec3 vertex6(center.x - length, center.y + length, center.z + length);
		glm::vec3 vertex7(center.x + length, center.y + length, center.z + length);

		// 将8个顶点坐标放入一个std::vector列表中
		std::vector<glm::vec3> vertices = { vertex0, vertex1, vertex2, vertex3, vertex4, vertex5, vertex6, vertex7 };

		//cGlobalBuffer.Lights.emplace_back(center, vec3(100.0f, 100.0f, 100.0f));

		for (auto pos : vertices)
		{
			cGlobalBuffer.Lights.emplace_back(pos, vec3(100.0f, 100.0f, 100.0f));
		}
	}

	RTContext raytracerContext;
	{
		raytracerContext.Camera = CameraInfo{
			.Position = pos,
			.Foucs	  = focus,
			.Up		  = up,
			.Fov	  = fov
		};
		raytracerContext.RTScene			  = scene;
		raytracerContext.GlobalParameters	  = cGlobalBuffer;
		raytracerContext.RenderTargetColor	  = std::make_shared<PhysicalImage>(width, height, channels_num);
		raytracerContext.RenderTargetNormal	  = std::make_shared<PhysicalImage>(width, height, channels_num);
		raytracerContext.RenderTargetEmissive = std::make_shared<PhysicalImage>(width, height, channels_num);
		raytracerContext.RenderTargetLighting = std::make_shared<PhysicalImage>(width, height, channels_num);
		raytracerContext.RenderTargetAO		  = std::make_shared<PhysicalImage>(width, height, channels_num);
		raytracerContext.RenderTargetDepth	  = std::make_shared<PhysicalImage32F>(width, height, channels_num);
	}

	Raytracer raytracer(raytracerContext);

	BatchBuffer cBatchBuffer;
	{
		cBatchBuffer.WorldMatrix = TMat4x4(1.0f);
	}

	PBRRender pbrRender;

	std::thread rtRenderThread([&pbrRender, &raytracerContext, &raytracer]()
							   { 
								   raytracer.RenderAsync();
							   });


	const auto mouse_callback = [&](float xoffset, float yoffset)
	{
		auto mat = glm::lookAtRH(pos, focus, up);

		// float sensitivity = 0.05f; //灵敏度
		// xoffset *= sensitivity;
		// yoffset *= sensitivity;

		// yaw += xoffset;
		// pitch += yoffset;

		glm::vec3 cameraFront	  = glm::normalize(focus - pos);
		float	  raw			  = glm::degrees(glm::asin(cameraFront.y));
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

	Renderer(raytracerContext.RenderTargetColor.get(), mouse_callback);
	renderImage.SaveToFile("D:/x_render.png");

	rtRenderThread.join();

	rtcReleaseScene(scene);
	rtcReleaseDevice(device);
}