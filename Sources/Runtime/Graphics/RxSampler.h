
#pragma once

#include "Types.h"
#include "RxImage.h"


enum class RxSamplerType
{
	Point = 0,
	Linear,
};

enum class RxWrapMode
{
	Clamp,
	Repeat,
	Mirror,
};

class RxSampler
{
protected:
	RxSampler(RxWrapMode wrapModel);

public:
	static RxSampler* CreateSampler(RxSamplerType type = RxSamplerType::Linear, RxWrapMode wrapMode = RxWrapMode::Clamp);

	virtual ~RxSampler();

public:
	// default linear
	virtual Color4f ReadPixel(const PhysicalImage* image, float u, float v) const;

	virtual Color4f ReadPixel(const PhysicalImage* image, Vector2f uv) const
	{
		return ReadPixel(image, uv.x, uv.y);
	}

protected:
	RxWrapMode mWrapModel;
};

Color4f texture2D(const PhysicalImage* texture, Vector2f uv);

Color4f textureCube(const PhysicalImage* texture, Vector3f dir , int Lod = 0);

Color4f textureCubeLod(const PhysicalImage* texture, Vector3f dir, int Lod = 0);