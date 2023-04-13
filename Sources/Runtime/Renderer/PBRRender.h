#pragma once

#include "Graphics/PhysicalImage.h"
#include "Graphics/Material.h"
#include "Graphics/Prefilter.h"

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
		mBRDFTexture = std::make_shared< Texture2D>("Engine/IBL_BRDF.png");
		//mEnvTexture	 = std::make_shared<>("SkyBox0");

		/*PhysicalImage32F image("Textures/hdr/newport_loft.hdr");
		mEnvTexture = PrefilterEnvironmentTexture(image);*/
		mEnvTexture = std::make_shared<TextureCube>("SkyBox0");

		mSphericalEnvTexture = std::make_shared<Texture2D>("Textures/hdr/newport_loft.hdr");
		mSphericalEnvTexture->AutoGenerateMipmaps();
	}

	virtual ~Renderer()
	{
	}

	virtual Color4f Render(const GlobalConstantBuffer& cGlobalBuffer, const BatchBuffer& cBatchBuffer, const ShadingBuffer& cShadingBuffer, Vector3f pos, Vector3f normal, Vector2f uv, const TMat3x3& normalMatrix, const Material* material) noexcept = 0;

	virtual Color4f RenderSkybox(const GlobalConstantBuffer& cGlobalBuffer, Vector3f worldPosition) noexcept = 0;

protected:
	TexturePtr mBRDFTexture;
	TexturePtr mEnvTexture;
	TexturePtr mSphericalEnvTexture;
};


class PBRRender: public Renderer
{
public:
	virtual Color4f Render(const GlobalConstantBuffer& cGlobalBuffer, const BatchBuffer& cBatchBuffer, const ShadingBuffer& cShadingBuffer, Vector3f pos, Vector3f normal, Vector2f uv, const TMat3x3& normalMatrix, const Material* material) noexcept override;

	virtual Color4f RenderSkybox(const GlobalConstantBuffer& cGlobalBuffer, Vector3f worldPosition) noexcept override;
};