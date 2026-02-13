#pragma once

#include "Graphics/PhysicalImage.h"
#include "Renderer/PBRRender.h"
#include "Camera.h"
#include <random>


#define HITBOCKSIZE 8

#if HITBOCKSIZE == 1
#	define GET_RTC_PARAM(ARGS, INDEX) (ARGS)
#else
#	define GET_RTC_PARAM(ARGS, INDEX) (ARGS[INDEX])
#endif

#if HITBOCKSIZE == 8
#	define RAY_TYPE						 RTCRayHit8
#	define RAY_INTERSECT(MASK, SCENE, RYAS) rtcIntersect8(MASK, SCENE, &RYAS)
#elif HITBOCKSIZE == 4
#	define RAY_TYPE						 RTCRayHit4
#	define RAY_INTERSECT(MASK, SCENE, RYAS) rtcIntersect4(MASK, SCENE, &RYAS)
#elif HITBOCKSIZE == 1
#	define RAY_TYPE						 RTCRayHit
#	define RAY_INTERSECT(MASK, SCENE, RYAS) rtcIntersect1(SCENE, &RYAS)
#else
static_assert(false);
#endif

struct RxRay
{
	uint32	 Mask = 0xFFFFFFFF;
	float	 MaxDistance;
	Vector3f Origion;
	Vector3f Direction; // normalized
};

struct RxRayHit : public RTCRayHit
{
};

struct RayHitResult
{
	bool			 IsHit = false;
	Material*		 Mat   = nullptr;
	VertexOutputData VertexData;
};

FORCEINLINE float GenerateRandom()
{
	static std::random_device				rd;
	static std::mt19937						gen(rd());
	static std::uniform_real_distribution<> dis(0, 1);

	return dis(gen);
}

struct RTContext
{
	CameraInfo			 Camera;
	GlobalConstantBuffer GlobalParameters;
	RTCScene			 RTScene = nullptr;

	std::shared_ptr<PhysicalImage> RenderTargetColor;
	std::shared_ptr<PhysicalImage> RenderTargetNormal;
	std::shared_ptr<PhysicalImage> RenderTargetDepth;
	std::shared_ptr<PhysicalImage> RenderTargetEmissive;
	std::shared_ptr<PhysicalImage> RenderTargetAO;
	std::shared_ptr<PhysicalImage> RenderTargetLighting;

	embree::Camera ToEmbreeCamera() const noexcept
	{
		embree::Camera camera;
		camera.from		  = embree::Vec3fa(Camera.Position.x, Camera.Position.y, Camera.Position.z);
		camera.to		  = embree::Vec3fa(Camera.Foucs.x, Camera.Foucs.y, Camera.Foucs.z);
		camera.up		  = embree::Vec3fa(Camera.Up.x, Camera.Up.y, Camera.Up.z);
		camera.fov		  = Camera.Fov;
		camera.handedness = embree::Camera::Handedness::RIGHT_HANDED;
		return camera;
	}
};

struct RxIntersection
{
	bool	 IsHit;
	RxRay	 Ray;
	uint32	 InstanceID;
	uint32	 GeomID;
	uint32	 PrimID;
	Vector2f Barycenter;

	vec2 SampleBRDF(const RTContext& context) noexcept;

	void SampleAttributes(const RTContext& context, VertexOutputData& vertexData, vec3& albedo, float& roughness, float& metallic);
};

struct TiledTaskData
{
	uint16 SW = 0; // start of height;
	uint16 SH = 0; // start of height;
};

std::vector<TiledTaskData> GenerateTasks(size_t width, size_t height, size_t tileSize);

class Raytracer
{
public:
	Raytracer(const RTContext& context);

	virtual ~Raytracer()
	{
	}

public:
	virtual void RenderAsync() noexcept;

	bool IsShadowRay(vec3 from, vec3 to) const noexcept;

	RxIntersection Intersection(const RxRay& ray) const noexcept;

public:
	static vec3 GetReflection(vec3 Li, vec3 Normal, float rougness) noexcept;
	static vec3 RandomInUnitSphere() noexcept;

protected:
	void ProcessTask(const TiledTaskData& task, const embree::ISPCCamera& ispcCamera) noexcept;

	static RAY_TYPE SetupRays(const std::vector<std::pair<int, int>>& pixels, const embree::ISPCCamera& ispcCamera, size_t startIndex, int* validMasks);

	RayHitResult GetRayHitResult(const RxRayHit& hit) noexcept;
	

protected:
	static const int NUM_CORE = 16;
	static const int TileSize = 64;
	RTContext mContext;
	embree::ISPCCamera mISPCCamera;
};