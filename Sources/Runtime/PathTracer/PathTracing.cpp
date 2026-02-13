
#include "PathTracing.h"
#include <span>
#include <random>
#include <glm/gtc/random.hpp>

//
// float GenerateRandom()
//{
//	static std::random_device				rd;
//	static std::mt19937						gen(rd());
//	static std::uniform_real_distribution<> dis(0, 1);
//
//	return dis(gen);
//}

PathTracing::PathTracing(const RTContext& context)
	: Raytracer(context)
{
}


void PathTracing::RenderAsync() noexcept
{
	SCOPED_PROFILING_GUARD("PathTracing");
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

#pragma omp parallel for num_threads(NUM_CORE) reduction(+ \
														 : count)
	for (int taskId = 0; taskId < NUM_CORE; ++taskId)
	{
		do
		{
			uint32 taskIdx = renderTileIndex.fetch_add(1, std::memory_order_acq_rel);
			if (taskIdx >= renderTiles.size())
			{
				break;
			}
			_ProcessTileTask(renderTiles[taskIdx], ispcCamera);
		} while (1);
	}
}

void PathTracing::_ProcessTileTask(const TiledTaskData& task, const embree::ISPCCamera& ispcCamera) noexcept
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

	for (const auto& [w, h] : pixels)
	{
		RxRayHit ray = _SetupRay(w, h, ispcCamera);

		Color4f color = _TraceRay(ray, 0);

		/*RenderOutputData renderOutputData;
		bool			 isValidSurface = _ProcessRayHitResult(this, mContext.GlobalParameters, vec3(x, y, z), vec2(u, v), vec3(ng_x, ng_y, ng_z), instanceId, geomID, primID, renderOutputData);*/

		
		{
			mContext.RenderTargetColor.get()->WritePixel(w, h, color);
			//mContext.RenderTargetDepth.get()->WritePixel(w, h, vec4(vec3(renderOutputData.Depth), 1.0f));
		}
	}
}

RxRayHit PathTracing::_SetupRay(int w, int h, const embree::ISPCCamera& ispcCamera) noexcept
{
	RxRayHit rayhits;

	auto dir	  = embree::Vec3fa(embree::normalize(w * ispcCamera.xfm.l.vx + h * ispcCamera.xfm.l.vy + ispcCamera.xfm.l.vz));

	rayhits.ray.org_x  = ispcCamera.xfm.p.x;
	rayhits.ray.org_y  = ispcCamera.xfm.p.y;
	rayhits.ray.org_z  = ispcCamera.xfm.p.z;
	rayhits.ray.dir_x  = dir.x;
	rayhits.ray.dir_y  = dir.y;
	rayhits.ray.dir_z  = dir.z;
	rayhits.ray.tnear  = 0.f;
	rayhits.ray.tfar   = 100000;
	rayhits.ray.time   = HITBOCKSIZE;
	rayhits.ray.mask   = 0xFFFFFFFF;
	rayhits.hit.geomID = RTC_INVALID_GEOMETRY_ID;

	return rayhits;
}

Color4f PathTracing::_TraceRay(RxRayHit primaryray, int depth) noexcept
{
	// max depth
	if (depth >= 5)
	{
		return { 0, 0, 0, 1 };
	}

	rtcIntersect1(mContext.RTScene, &primaryray);

	RayHitResult result = GetRayHitResult(primaryray);
	if (!result.IsHit)
	{
		return vec4(0, 0, 0, 1.0);
	}

	auto* material = result.Mat;

	RenderOutputData output = material->GetRenderCore()->Execute(this, mContext.GlobalParameters, result.VertexData, material);

	return output.Color;
}