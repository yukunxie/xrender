
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

	EnvironmentTextures gEnvironmentData;
	{
		gEnvironmentData.BRDFTexture = mBRDFTexture;
		gEnvironmentData.EnvTexture	 = mEnvTexture;
		gEnvironmentData.SphericalEnvTexture = mSphericalEnvTexture;
	}


	return PBRShading(cGlobalBuffer, gBufferData, gEnvironmentData);
}

static vec2 _SampleSphericalMap(vec3 v)
{
	static const vec2 invAtan = vec2(0.1591f, 0.3183f);
	vec2 uv = vec2(glm::atan(v.z, v.x), asin(v.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

Color4f PBRRender::RenderSkybox(const GlobalConstantBuffer& cGlobalBuffer, Vector3f worldPosition) noexcept
{
#if 1
	glm::vec4 viewPos	 = cGlobalBuffer.ViewMatrix * glm::vec4(worldPosition, 1.0f);
	glm::vec3 viewCoords = viewPos.xyz;
	viewCoords			 = viewCoords / viewPos.w;

	glm::mat4 skyboxViewMatrix = glm::mat4(glm::mat3(cGlobalBuffer.ViewMatrix));
	glm::vec3 skyboxCoords = glm::vec3(skyboxViewMatrix * glm::vec4(viewCoords, 1.0f));

	//return { 0.2f, 0.2f, 0, 1 };
	vec3 color = textureCubeLod(mEnvTexture.get(), worldPosition.xyz, 0).rgb;
	//color	   = glm::pow(color, vec3(2.2f));

	//	// HDR tonemapping
	// color = color / (color + vec3(1.0));
	//// gamma correct
	// color = pow(color, vec3(1.0 / 2.2));

	 auto FragColor = vec4(color, 1.0);
	 return FragColor;
#else
	vec2 uv = _SampleSphericalMap(glm::normalize(worldPosition));
	return texture2D(mSphericalEnvTexture, vec2(uv.x, 1.0f - uv.y), 0);
#endif


	//return Color4f(prefilteredColor, 1.0f);
	//return Color4f{ 0, (worldPosition.y + 1000.0f) / 2000.0f, (worldPosition.z + 1000.0f)/ 2000.0f, 1.0f };
}