
#include "PBRRender.h"
#include "Graphics/RxSampler.h"
#include <algorithm>
#include "PBRShader.h"
#include "ShadingBase.h"



/*

- Sampling the GGX Distribution of Visible Normals
  http://jcgt.org/published/0007/04/01/

- Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs
  http://jcgt.org/published/0003/02/03/

- Microfacet Models for Refraction through Rough Surfaces
  https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.html

*/

Color4f PBRRender::Render(const GlobalConstantBuffer& cGlobalBuffer, const BatchBuffer& cBatchBuffer, const ShadingBuffer& cShadingBuffer, Vector3f pos, Vector3f worldNormal, Vector2f uv, const TMat3x3& normalMatrix, const Material* material) noexcept
{
	auto texture = material->GetTexture("tAlbedo");
	auto matTexture = material->GetTexture("tMetallicRoughnessMap");
	auto normalTexture = material->GetTexture("tNormalMap");

	 static RxSampler* sampler = RxSampler::CreateSampler(RxSamplerType::Linear, RxWrapMode::Repeat);

	 Color3f Albedo = Color3f{ cShadingBuffer.baseColorFactor };
	 if (texture)
	 {
		 Albedo = Albedo * sampler->ReadPixel(texture.get(), uv.x, uv.y).xyz;
	 }

	Vector3f sunDir	 = -1.0f * Vector3f{ cGlobalBuffer.SunLight.x, cGlobalBuffer.SunLight.y, cGlobalBuffer.SunLight.z };
	sunDir			 = glm::normalize(sunDir);
	Vector3f viewDir = Vector3f(cGlobalBuffer.EyePos) - pos;
	viewDir			 = glm::normalize(viewDir);

	vec3 N;
	if (normalTexture)
	{
		vec3 tn = vec3(sampler->ReadPixel(normalTexture.get(), uv.x, uv.y));
		N		= glm::normalize(normalMatrix * (tn * 2.0f - 1.0f));
	}
	else
	{
		N = glm::normalize(worldNormal);
	}

	// 计算视线方向Metallic
	vec3 V = glm::normalize(viewDir);

	// 计算光线方向和半角向量
	vec3 L = sunDir;
	vec3 H = normalize(L + V);
	vec3 R = reflect(-V, N); 

	vec4  mat11		= sampler->ReadPixel(matTexture.get(), uv.x, uv.y);
	float roughness = mat11.g;
	float metallic	= mat11.b;

	float Roughness = roughness;
	float Metallic  = metallic;

	//return PBRShading(metallic, roughness, normalMatrix, N, V, L, H, 1.0f, Albedo, mBRDFTexture.get(), (RxImageCube*)mEnvTexture.get());

	GBufferData gBufferData{
		.Albedo		 = Albedo,
		.WorldNormal = N,
		.Position	 = pos,
		.Material	 = vec3(metallic, roughness, 0)
	};

	return PBRShading(cGlobalBuffer, gBufferData, mBRDFTexture.get(), (RxImageCube*)mEnvTexture.get());
}