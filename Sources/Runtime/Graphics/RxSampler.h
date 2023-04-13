
#pragma once

#include "Types.h"
#include "Texture.h"


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
	virtual Color4f ReadPixel(const Texture* image, float u, float v, int lod = 0) const;

	virtual Color4f ReadPixel(const Texture* image, Vector2f uv, int lod = 0) const
	{
		return ReadPixel(image, uv.x, uv.y, lod);
	}

protected:
	RxWrapMode mWrapModel;
};

Color4f texture2D(const Texture* texture, Vector2f uv, float lod = 0);

Color4f textureCube(const Texture* texture, Vector3f dir, float lod = 0);

Color4f textureCubeLod(const Texture* texture, Vector3f dir, float lod = 0);

FORCEINLINE Color4f texture2D(const TexturePtr& texture, Vector2f uv, float lod = 0)
{
	return texture2D(texture.get(), uv, lod);
}

FORCEINLINE Color4f textureCube(const TexturePtr& texture, Vector3f dir, float lod = 0)
{
	return textureCube(texture.get(), dir, lod);
}

FORCEINLINE Color4f textureCubeLod(const TexturePtr& texture, Vector3f dir, float lod = 0)
{
	return textureCubeLod(texture.get(), dir, lod);
}
