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
#include "PathTracer/Raytracer.h"
#include "PathTracer/PhotonMapping.h"
#include "Renderer/PBRRender.h"

std::map<int, RTInstanceData> GMeshComponentProxies;

void AddEntityToEmbreeScene(RTCDevice device_i, RTCScene scene_i, const std::vector<Entity*>& entities)
{
	for (auto entity : entities)
	{
		entity->AddToEmbreeScene(device_i, scene_i);
	}
}

int width  = 1024;
int height = 1024;

vec3 GetRandomColor()
{
	static std::random_device				rd;
	static std::mt19937						gen(32);
	static std::uniform_real_distribution<> dis(0, 1);

	float r1 = dis(gen);
	float r2 = dis(gen);
	float r3 = dis(gen);

	return vec3(r1, r2, r3);
}


int main()
{
	std::cout << "Please use CounterClockWise CCW Triangle and Right Handness" << std::endl;
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
		auto	cubeMesh = MeshComponentBuilder::CreatePlane();
		Entity* plane	 = new Entity();
		plane->AddComponment(cubeMesh);
		std::vector<Entity*> entities = { plane };
		entities[0]->SetScale(vec3(2000, 1.0f, 2000));
		entities[0]->SetPosition(vec3(0, -1.0f, 0));
		MeshComponent* mesh		 = entities[0]->GetComponent<MeshComponent>();
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

	Vector3f pos = { 10, 2.0f, 0.0f };
	// Vector3f pos   = { 50, 10.0f, 0.0f };
	Vector3f			 focus = { .0f, 2.0f, 0.0f };
	Vector3f			 up	   = { 0, 1, 0 };
	float				 fov   = 60;


	GlobalConstantBuffer cGlobalBuffer;
	{
		cGlobalBuffer.EyePos		 = Vector4f(pos.x, pos.y, pos.z, 1.0f);
		cGlobalBuffer.SunLight		 = Vector4f(.0f, -1.0f, 0.0f, 0.0f);
		cGlobalBuffer.SunLightColor	 = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
		cGlobalBuffer.ViewMatrix	 = glm::lookAtRH(pos, focus, up);
		cGlobalBuffer.ProjMatrix	 = glm::perspectiveFovRH(glm::radians(fov), float(width), float(height), 0.1f, 200.0f);
		cGlobalBuffer.ViewProjMatrix = cGlobalBuffer.ProjMatrix * cGlobalBuffer.ViewMatrix;


		glm::vec3 center(0.0f, 3.0f, 0.0f); // 立方体中心位置
		float	  length = 5.0f;			// 立方体边长的一半
		vec3	  pointLightColor = vec3(100.0f, 100.0f, 100.0f);

		for (int i = 0; i < 5; ++i)
		{
			for (int j = 0; j < 5; ++j)
			{
				glm::vec3 vertex0(center.x + length * (i - 2), center.y, center.z + length * (j - 2));
				vec3	  randomColor = GetRandomColor();
				cGlobalBuffer.Lights.emplace_back(vertex0, pointLightColor , randomColor);
			}
		}


#if 0
		// create point light shape
		for (const auto& light : cGlobalBuffer.Lights)
		{
			auto	lightShape = MeshComponentBuilder::CreateSphere();
			Material *mat = lightShape->GetMaterial();
			vec4	  baseColor	 = vec4(light.ShapeColor.xyz, 1.0f);
			mat->SetFloat("baseColorFactor", 0, 4, &baseColor.x);
			vec4 emissiveColor = vec4(0);
			mat->SetFloat("emissiveFactor", 0, 4, &emissiveColor.x);
			lightShape->GetMaterial()->SetRenderCore(std::make_shared<RenderCoreUnlit>());
			Entity* entity	 = new Entity();
			entity->AddComponment(lightShape);
			entity->SetScale(vec3{ 0.2f });
			entity->SetPosition(light.Position);
			entity->SetCastShadow(false);
			entity->SetRecieveShadow(false);
			AddEntityToEmbreeScene(device, scene, { entity });

			const auto& wp = entity->GetWorldMatrix();

			vec4 z = wp * vec4(0, 0, 0, 1.0f);
			if (true)
				;
		}
#endif
	}

	rtcCommitScene(scene);


	 //print_bvh(scene);

	int			  channels_num = 4;
	PhysicalImage renderImage(width, height, channels_num);

	auto sampler = RxSampler::CreateSampler();

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

#if 1
	Raytracer raytracer(raytracerContext);
#else
	PhotonMapper raytracer(raytracerContext);
#endif

	BatchBuffer cBatchBuffer;
	{
		cBatchBuffer.WorldMatrix = TMat4x4(1.0f);
	}

	PBRRender pbrRender;

	std::thread rtRenderThread([&pbrRender, &raytracerContext, &raytracer]()
							   { 
								   raytracer.RenderAsync();

								   PhotonMapper raytracer2(raytracerContext);
								   raytracer2.RenderAsync();
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