
#include "Raytracer.h"

#include <embree4/rtcore.h>
#include <random>
#include <limits>
#include <iostream>
#include <thread>
#include <simd/varying.h>
#include<omp.h> 

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
#include "Object/MeshComponent.h"
#include "Object/Entity.h"
#include <glm/glm.hpp>
#include <glm/geometric.hpp>  

#define HITBOCKSIZE 8

#if HITBOCKSIZE == 1
#	define GET_RTC_PARAM(ARGS, INDEX) (ARGS)
#else
#	define GET_RTC_PARAM(ARGS, INDEX) (ARGS[INDEX])
#endif

#if HITBOCKSIZE == 8
#	define RAY_TYPE RTCRayHit8
#	define RAY_INTERSECT(MASK, SCENE, RYAS) rtcIntersect8(MASK, SCENE, &RYAS)
#elif HITBOCKSIZE == 4
#	define RAY_TYPE RTCRayHit4
#	define RAY_INTERSECT(MASK, SCENE, RYAS) rtcIntersect4(MASK, SCENE, &RYAS)
#elif HITBOCKSIZE == 1
#	define RAY_TYPE RTCRayHit
#	define RAY_INTERSECT(MASK, SCENE, RYAS) rtcIntersect1(SCENE, &RYAS)
#else
    static_assert(false);
#endif


extern std::map<int, RTInstanceData> GMeshComponentProxies;

glm::vec3 triangleBitangent(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3)
{
	glm::vec3 edge1	   = v2 - v1;
	glm::vec3 edge2	   = v3 - v1;
	glm::vec2 deltaUV1 = uv2 - uv1;
	glm::vec2 deltaUV2 = uv3 - uv1;

	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	glm::vec3 bitangent;
	bitangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	bitangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	bitangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	return bitangent;
}

glm::vec3 triangleTangent(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3)
{
	glm::vec3 edge1	   = v2 - v1;
	glm::vec3 edge2	   = v3 - v1;
	glm::vec2 deltaUV1 = uv2 - uv1;
	glm::vec2 deltaUV2 = uv3 - uv1;

	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	glm::vec3 tangent;
	tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	return tangent;
}

Vector3f interpolateNormal(float u, float v, Vector3f p0, Vector3f p1, Vector3f p2, Vector3f n0, Vector3f n1, Vector3f n2)
{
	glm::vec3 p = (1 - u - v) * p0 + u * p1 + v * p2;
	glm::vec3 tangent0, bitangent0, tangent1, bitangent1, tangent2, bitangent2;
	//glm::triangleTangent(p0, p1, p2, tangent0, bitangent0);
	//glm::triangleTangent(p1, p2, p0, tangent1, bitangent1);
	//glm::triangleTangent(p2, p0, p1, tangent2, bitangent2);

	return {};
}

static bool _ProcessRayHitResult(const Raytracer* rayTracer, const GlobalConstantBuffer& cGlobalBuffer, vec3 viewDir, vec2 uv, vec3 ng, int instanceId, int geomID, int primID, RenderOutputData & output)
{
	if (instanceId < 0 || geomID < 0 || primID < 0)
	{
		output.Color = Color4f(1.0f, 0.0f, 1.0f, 1.0f);
		return true;
	}
	else
	{
		auto meshProxy = GMeshComponentProxies[instanceId].MeshComponents[geomID];

		Material* material = meshProxy->GetMaterial();

		float u = uv.x;
		float v = uv.y;

		uint32 v0, v1, v2;
		std::tie(v0, v1, v2) = meshProxy->GetGeometry()->GetIndexBuffer()->GetVerticesByPrimitiveId(primID);

		const TMat4x4 & worldMatrix =  meshProxy->GetOwner()->GetWorldMatrix();

		const VertexOutputData& vertexData = RenderCore::InterpolateAttributes(vec2(u, v), ng, worldMatrix, meshProxy->GetGeometry(), primID);

		Vector3f p0, p1, p2;
		meshProxy->GetGeometry()->GetTripleAttributesByIndex<Vector3f>(VertexBufferAttriKind::NORMAL, v0, v1, v2, p0, p1, p2);

		vec3 N = glm::cross(p1 - p0, p2 - p0);

		bool isBackSurface = false;
		if (glm::dot( cGlobalBuffer.EyePos.xyz - vertexData.Position, N) < 0.0f)
		{
			isBackSurface = true;
		}

		output = material->GetRenderCore()->Execute(rayTracer, cGlobalBuffer, vertexData, material);
		return !isBackSurface || material->GetDoubleSide();
	}
}

std::vector<TiledTaskData> GenerateTasks(size_t width, size_t height, size_t tileSize)
{
	std::vector<TiledTaskData> renderTiles;
	std::atomic_uint32_t  renderTileIndex = 0;

	for (uint16 h = 0; h < height; h += tileSize)
	{
		for (uint16 w = 0; w < width; w += tileSize)
		{
			renderTiles.emplace_back(w, h);
		}
	}

	std::random_device rd;
	std::mt19937	   g(rd());
	std::shuffle(renderTiles.begin(), renderTiles.end(), g);

	return std::move(renderTiles);
}

Raytracer::Raytracer(const RTContext& context)
	: mContext(context)
	, mISPCCamera(embree::AffineSpace3fa())
{
}

void Raytracer::RenderAsync() noexcept
{
	SCOPED_PROFILING_GUARD("RayTracingRender");
	int width  = mContext.RenderTargetColor->GetWidth();
	int height = mContext.RenderTargetColor->GetHeight();

	embree::Camera	   camera	  = mContext.ToEmbreeCamera();
	embree::ISPCCamera ispcCamera = camera.getISPCCamera(width, height);
	mISPCCamera					  = ispcCamera;
	
	constexpr int			   TileSize		   = 64;
	constexpr int			   NUM_CORE		   = 16;
	std::atomic_uint32_t	   renderTileIndex = 0;
	std::vector<TiledTaskData> renderTiles	   = GenerateTasks(width, height, TileSize);

	int count = 0;

#pragma omp parallel for num_threads(NUM_CORE) reduction(+ : count)
	for (int taskId = 0; taskId < NUM_CORE; ++taskId)
	{
		do
		{
			uint32 taskIdx = renderTileIndex.fetch_add(1, std::memory_order_acq_rel);
			if (taskIdx >= renderTiles.size())
			{
				break;
			}
			ProcessTask(renderTiles[taskIdx], ispcCamera);
		} while (1);
	}
}

RAY_TYPE SetupRays(const std::vector<std::pair<int, int>>& pixels, const embree::ISPCCamera& ispcCamera, size_t startIndex, int* validMasks)
{
	RAY_TYPE rayhits;

	for (int b = 0; b < HITBOCKSIZE; ++b)
	{
		if (b + startIndex < pixels.size())
		{
			int w = pixels[b + startIndex].first;
			int h = pixels[b + startIndex].second;

			validMasks[b] = 0xFFFFFFFF;
			auto dir	  = embree::Vec3fa(embree::normalize(w * ispcCamera.xfm.l.vx + h * ispcCamera.xfm.l.vy + ispcCamera.xfm.l.vz));

			GET_RTC_PARAM(rayhits.ray.org_x, b)	 = ispcCamera.xfm.p.x;
			GET_RTC_PARAM(rayhits.ray.org_y, b)	 = ispcCamera.xfm.p.y;
			GET_RTC_PARAM(rayhits.ray.org_z, b)	 = ispcCamera.xfm.p.z;
			GET_RTC_PARAM(rayhits.ray.dir_x, b)	 = dir.x;
			GET_RTC_PARAM(rayhits.ray.dir_y, b)	 = dir.y;
			GET_RTC_PARAM(rayhits.ray.dir_z, b)	 = dir.z;
			GET_RTC_PARAM(rayhits.ray.tnear, b)	 = 0.f;
			GET_RTC_PARAM(rayhits.ray.tfar, b)	 = 100000;
			GET_RTC_PARAM(rayhits.ray.time, b)	 = HITBOCKSIZE;
			GET_RTC_PARAM(rayhits.ray.mask, b)	 = 0xFFFFFFFF;
			GET_RTC_PARAM(rayhits.hit.geomID, b) = RTC_INVALID_GEOMETRY_ID;
		}
		else
		{
			validMasks[b] = 0x0;
		}
	}
	return rayhits;
}

void Raytracer::ProcessTask(const TiledTaskData& task, const embree::ISPCCamera& ispcCamera) noexcept
{
	int width  = mContext.RenderTargetColor->GetWidth();
	int height = mContext.RenderTargetColor->GetHeight();

	std::vector<std::pair<int, int>> pixels;
	for (int h = task.SH; h < task.SH + TileSize; ++h)
	{
		for (int w = task.SW; w < task.SW + TileSize; ++w)
		{
			if (h < height && w < width)
			{
				pixels.emplace_back(w, h);
			}
		}
	}

	alignas(64) int validMasks[HITBOCKSIZE];
	memset(validMasks, 0xFF, sizeof(validMasks));

	for (int t = 0; t < pixels.size(); t += HITBOCKSIZE)
	{
		RAY_TYPE rayhits = SetupRays(pixels, ispcCamera, t, validMasks);
		
		RAY_INTERSECT(validMasks, mContext.RTScene, rayhits);

		for (int b = 0; b < HITBOCKSIZE; ++b)
		{
			int w = pixels[b + t].first;
			int h = pixels[b + t].second;

			float x = GET_RTC_PARAM(rayhits.ray.dir_x, b);
			float y = GET_RTC_PARAM(rayhits.ray.dir_y, b);
			float z = GET_RTC_PARAM(rayhits.ray.dir_z, b);

			// The triange is CCW
			float u = GET_RTC_PARAM(rayhits.hit.u, b);
			float v = GET_RTC_PARAM(rayhits.hit.v, b);

			int		primID	   = GET_RTC_PARAM(rayhits.hit.primID, b);
			int		geomID	   = GET_RTC_PARAM(rayhits.hit.geomID, b);
			int		instanceId = GET_RTC_PARAM(rayhits.hit.instID[0], b);
			float	ng_x	   = GET_RTC_PARAM(rayhits.hit.Ng_x, b);
			float	ng_y	   = GET_RTC_PARAM(rayhits.hit.Ng_y, b);
			float	ng_z	   = GET_RTC_PARAM(rayhits.hit.Ng_z, b);
			RenderOutputData renderOutputData;
			bool			 isValidSurface = _ProcessRayHitResult(this, mContext.GlobalParameters, vec3(x, y, z), vec2(u, v), vec3(ng_x, ng_y, ng_z), instanceId, geomID, primID, renderOutputData);

			/*auto ViewMatrix = mContext.GlobalParameters.ViewMatrix;
			auto ProjMatrix = mContext.GlobalParameters.ProjMatrix;
			auto mvp		= ProjMatrix * ViewMatrix;
			vec4 np			= mvp * vec4(renderOutputData.WorldPosition, 1.0f);
			np /= np.w;
			np.xyz = np.xyz * 0.5f + 0.5f;
			w	   = glm::clamp(int(np.x * width), 0, width - 1);
			h	   = glm::clamp(int((1 - np.y) * height), 0, height - 1);*/
			
			// if (!isValidSurface)
			{
				mContext.RenderTargetColor.get()->WritePixel(w, h, renderOutputData.Color);
				mContext.RenderTargetDepth.get()->WritePixel(w, h, vec4(vec3(renderOutputData.Depth), 1.0f));
			}
			
		}
	}
}

bool Raytracer::IsShadowRay(vec3 from, vec3 to) const noexcept
{
	RTCRayHit rayhit;
	vec3	  dir	  = to - from;
	rayhit.ray.org_x  = from.x;
	rayhit.ray.org_y  = from.y;
	rayhit.ray.org_z  = from.z;
	rayhit.ray.dir_x  = dir.x;
	rayhit.ray.dir_y  = dir.y;
	rayhit.ray.dir_z  = dir.z;
	rayhit.ray.tnear  = 0.000001f;
	rayhit.ray.tfar	  = glm::length(dir);
	rayhit.ray.time	  = 0;
	rayhit.ray.mask	  = 0xFFFFFFFF;
	rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
	rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

	rtcIntersect1(mContext.RTScene, &rayhit);

	return rayhit.hit.instID[0] != 0 && (rayhit.hit.instID[0] != RTC_INVALID_GEOMETRY_ID);
}

RxIntersection Raytracer::Intersection(const RxRay& ray) const noexcept
{
	RTCRayHit rayhit;
	vec3	  dir		 = ray.Direction;
	rayhit.ray.org_x	 = ray.Origion.x;
	rayhit.ray.org_y	 = ray.Origion.y;
	rayhit.ray.org_z	 = ray.Origion.z;
	rayhit.ray.dir_x	 = ray.Direction.x;
	rayhit.ray.dir_y	 = ray.Direction.y;
	rayhit.ray.dir_z	 = ray.Direction.z;
	rayhit.ray.tnear	 = 0.0f;
	rayhit.ray.tfar		 = ray.MaxDistance;
	rayhit.ray.time		 = 1;
	rayhit.ray.flags	 = 0;
	rayhit.ray.mask		 = ray.Mask;
	rayhit.hit.geomID	 = RTC_INVALID_GEOMETRY_ID;
	rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

	rtcIntersect1(mContext.RTScene, &rayhit);

	RxIntersection result;
	result.Ray		  = ray;
	result.InstanceID = rayhit.hit.instID[0];
	result.GeomID	  = rayhit.hit.geomID;
	result.PrimID	  = rayhit.hit.primID;
	result.Barycenter = { rayhit.hit.u, rayhit.hit.v };

	result.IsHit = rayhit.hit.instID[0] != RTC_INVALID_GEOMETRY_ID && (rayhit.hit.instID[0] != RTC_INVALID_GEOMETRY_ID);

	return result;
}

vec2 RxIntersection::SampleBRDF(const RTContext& context) noexcept
{
	auto	  meshProxy = GMeshComponentProxies[InstanceID].MeshComponents[GeomID];
	Material* material	= meshProxy->GetMaterial();

	const TMat4x4&			worldMatrix = meshProxy->GetOwner()->GetWorldMatrix();
	const VertexOutputData& vertexData	= RenderCore::InterpolateAttributes(Barycenter, vec3(1, 1, 1), worldMatrix, meshProxy->GetGeometry(), PrimID);

	auto abledoTexture	  = material->GetTexture("tAlbedo");
	auto matTexture		  = material->GetTexture("tMetallicRoughnessMap");
	auto normalTexture	  = material->GetTexture("tNormalMap");
	auto emissiveTexture  = material->GetTexture("tEmissiveMap");

	vec3 L = glm::normalize(Ray.Direction);
	vec3 N = vertexData.Normal;


	return vec2{0};
}

void RxIntersection::SampleAttributes(const RTContext& context, VertexOutputData& vertexData, vec3& albedo, float& roughness, float& metallic)
{
	auto	  meshProxy = GMeshComponentProxies[InstanceID].MeshComponents[GeomID];
	Material* material	= meshProxy->GetMaterial();

	const TMat4x4&			worldMatrix = meshProxy->GetOwner()->GetWorldMatrix();
	vertexData	= RenderCore::InterpolateAttributes(Barycenter, vec3(1, 1, 1), worldMatrix, meshProxy->GetGeometry(), PrimID);

	auto abledoTexture	 = material->GetTexture("tAlbedo");
	auto matTexture		 = material->GetTexture("tMetallicRoughnessMap");
	auto normalTexture	 = material->GetTexture("tNormalMap");
	auto emissiveTexture = material->GetTexture("tEmissiveMap");

	static RxSampler* sampler = RxSampler::CreateSampler(RxSamplerType::Linear, RxWrapMode::Repeat);

	albedo = sampler->ReadPixel(abledoTexture.get(), Barycenter.x, Barycenter.y).xyz;

	if (matTexture)
	{
		vec4 mat11 = sampler->ReadPixel(matTexture.get(), Barycenter.x, Barycenter.y);
		roughness  = mat11.g;
		metallic   = mat11.b;
	}
	else
	{
		roughness = material->GetFloat("roughnessFactor");
		metallic  = material->GetFloat("metallicFactor");
	}
}