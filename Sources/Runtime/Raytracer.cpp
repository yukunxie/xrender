
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


extern std::map<int, MeshComponent*> GMeshComponentProxies;

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

static bool _ProcessRayHitResult(/*PBRRender& pbrRender, */const GlobalConstantBuffer& cGlobalBuffer, vec3 viewDir, vec2 uv, int geomID, int primID, Color4f & fragColor)
{
	//Assert(geomID != RTC_INVALID_GEOMETRY_ID);

	if (geomID == RTC_INVALID_GEOMETRY_ID)
	{
		fragColor = Color4f(1.0f, 0.0f, 1.0f, 1.0f);
		return true;
	}
	else /*if (geomID != 0)*/
	{
		auto  meshProxy = GMeshComponentProxies[geomID];

		//return Color4f(meshProxy->GetDebugColor(), 1.0f);

		Material* material = meshProxy->GetMaterial();

		float u = uv.x;
		float v = uv.y;

		uint32 v0, v1, v2;
		std::tie(v0, v1, v2) = meshProxy->GetGeometry()->GetIndexBuffer()->GetVerticesByPrimitiveId(primID);

		const VertexOutputData& vertexData = RenderCore::InterpolateAttributes(vec2(u, v), meshProxy->GetGeometry(), primID);

		Vector3f p0, p1, p2;
		std::tie(p0, p1, p2) = meshProxy->GetGeometry()->GetTripleAttributesByIndex<Vector3f>(VertexBufferAttriKind::NORMAL, v0, v1, v2);

		vec3 N = glm::cross(p1 - p0, p2 - p0);

		bool isBackSurface = false;
		if (glm::dot( cGlobalBuffer.EyePos.xyz - vertexData.Position, N) < 0.0f)
		{
			isBackSurface = true;
		}

		fragColor = material->GetRenderCore()->Execute(cGlobalBuffer, vertexData, material);
		return !isBackSurface || material->GetDoubleSide();
	}
	//else if (geomID == 0)
	//{
	//	/*auto meshProxy = GMeshComponentProxies[geomID];

	//	float u = uv.x;
	//	float v = uv.y;

	//	uint32 v0, v1, v2;
	//	std::tie(v0, v1, v2) = meshProxy->GetGeometry()->GetIndexBuffer()->GetVerticesByPrimitiveId(primID);

	//	Vector3f p0 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::POSITION, v0);
	//	Vector3f p1 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::POSITION, v1);
	//	Vector3f p2 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::POSITION, v2);

	//	Vector3f pos = u * p0 + v * p1 + (1 - u - v) * p2;

	//	fragColor = pbrRender.RenderSkybox(cGlobalBuffer, (pos));*/

	//	fragColor = Color4f(0.0, 0.0, 1.0, 1.0);
	//	return true;
	//}
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

void Raytracer::RenderAsync() noexcept
{
	SCOPED_PROFILING_GUARD("RayTracingRender");
	int width  = mContext.RenderTargetColor->GetWidth();
	int height = mContext.RenderTargetColor->GetHeight();

	embree::Camera camera = mContext.ToEmbreeCamera();
	embree::ISPCCamera ispcCamera = camera.getISPCCamera(width, height);
	
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

			// The triange is CCW£¬ so we need to invert the u and v
			float v = GET_RTC_PARAM(rayhits.hit.u, b);
			float u = GET_RTC_PARAM(rayhits.hit.v, b);

			int		primID = GET_RTC_PARAM(rayhits.hit.primID, b);
			int		geomID = GET_RTC_PARAM(rayhits.hit.geomID, b);
			Color4f fragColor;
			bool	isValidSurface = _ProcessRayHitResult(mContext.GlobalParameters, vec3(x, y, z), vec2(u, v), geomID, primID, fragColor);
			// if (!isValidSurface)
			{
				mContext.RenderTargetColor.get()->WritePixel(w, h, fragColor);
			}
		}
	}
}