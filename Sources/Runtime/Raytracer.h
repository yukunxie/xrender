#pragma once

#include "Graphics/PhysicalImage.h"
#include "Renderer/PBRRender.h"
#include "Camera.h"

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

public:
	void RenderAsync() noexcept;

	bool IsShadowRay(vec3 from, vec3 to) const noexcept;

protected:
	void ProcessTask(const TiledTaskData& task, const embree::ISPCCamera& ispcCamera) noexcept;


protected:
	static const int NUM_CORE = 16;
	static const int TileSize = 64;
	RTContext mContext;
	embree::ISPCCamera mISPCCamera;
};