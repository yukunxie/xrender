#pragma once

#include "Graphics/RxImage.h"
#include "Graphics/Material.h"

namespace GGX
{
	float	   D(const Vector3f& m, const Vector2f& a);
	float	   DV(const Vector3f& m, const Vector3f& wo, const Vector2f& a);
	float	   Lambda(const Vector3f& wo, const Vector2f& a);
	float	   SmithG1(const Vector3f& wo, const Vector2f& a);
	float	   SmithG2(const Vector3f& wi, const Vector3f& wo, const Vector2f& a);
	float	   reflection(const Vector3f& wi, const Vector3f& wo, const Vector2f& a, float& PDF);
	float	   transmission(const Vector3f& wi, const Vector3f& wo, float n1, float n2, const Vector2f& a, float& PDF);
	Vector3f visibleMicrofacet(float u, float v, const Vector3f& wo, const Vector2f& a);
} // namespace GGX


class Renderer
{
public:
	Renderer()
	{
		mBRDFTexture = std::make_shared< RxImage>("Engine/IBL_BRDF.png");
		mEnvTexture	 = std::make_shared<RxImageCube>("SkyBox0");
	}

	virtual ~Renderer()
	{
	}

	virtual Color4f Render(const GlobalConstantBuffer& cGlobalBuffer, const BatchBuffer& cBatchBuffer, const ShadingBuffer& cShadingBuffer, Vector3f pos, Vector3f normal, Vector2f uv, const Material* material) noexcept = 0;

protected:
	TexturePtr mBRDFTexture;
	TexturePtr mEnvTexture;
};


class PBRRender: public Renderer
{
public:
	virtual Color4f Render(const GlobalConstantBuffer& cGlobalBuffer, const BatchBuffer& cBatchBuffer, const ShadingBuffer& cShadingBuffer, Vector3f pos, Vector3f normal, Vector2f uv, const Material* material) noexcept override;
};