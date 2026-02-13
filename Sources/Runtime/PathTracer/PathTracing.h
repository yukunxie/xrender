#pragma once

#include "Raytracer.h"


class PathTracing: public Raytracer
{
public:
	PathTracing(const RTContext& context);

	virtual void RenderAsync() noexcept;

protected:
	void _ProcessTileTask(const TiledTaskData& task, const embree::ISPCCamera& ispcCamera) noexcept;

	RxRayHit _SetupRay(int w, int h, const embree::ISPCCamera& ispcCamera) noexcept;

	Color4f _TraceRay(RxRayHit primaryray, int depth) noexcept;
};