#pragma once

#include "Graphics/PhysicalImage.h"
#include "Graphics/Material.h"
#include "Graphics/Prefilter.h"

class Renderer
{
public:
	Renderer()
	{
		mBRDFTexture = std::make_shared< Texture2D>("Engine/lutBRDF.png");
		//mEnvTexture	 = std::make_shared<>("SkyBox0");

		/*PhysicalImage32F image("Textures/hdr/newport_loft.hdr");
		mEnvTexture = PrefilterEnvironmentTexture(image);*/
		mEnvTexture = std::make_shared<TextureCube>("SkyBox0");

		mIrradianceTexture = std::make_shared<TextureCube>("PreIrradiance0");

		mSphericalEnvTexture = std::make_shared<Texture2D>("Textures/hdr/newport_loft.hdr");
		mSphericalEnvTexture->AutoGenerateMipmaps();
	}

	virtual ~Renderer()
	{
	}

	virtual Color4f RenderSkybox(const GlobalConstantBuffer& cGlobalBuffer, Vector3f worldPosition) noexcept = 0;

protected:
	TexturePtr mBRDFTexture;
	TexturePtr mEnvTexture;
	TexturePtr mIrradianceTexture;
	TexturePtr mSphericalEnvTexture;
};


class PBRRender: public Renderer
{
public:
	virtual Color4f RenderSkybox(const GlobalConstantBuffer& cGlobalBuffer, Vector3f worldPosition) noexcept override;
};