
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
	virtual Color4f SamplePixel(const RxImage* image, float u, float v) const;

	virtual Color4f SamplePixel(const RxImage* image, Vector2f uv) const
	{
		return SamplePixel(image, uv.x, uv.y);
	}

protected:
	RxWrapMode mWrapModel;
};

Color4f texture2D(const RxImage* texture, Vector2f uv);

Color4f textureCube(const RxImage* texture, Vector3f dir , int Lod = 0);

Color4f textureCubeLod(const RxImage* texture, Vector3f dir, int Lod = 0);