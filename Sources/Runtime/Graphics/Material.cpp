#include "Material.h"
#include "Meshes/Geometry.h"
#include "VertexBuffer.h"
#include "Graphics/RxSampler.h"

EnvironmentTextures* GetEnvironmentData()
{
	static EnvironmentTextures GEnvironmentData;
	return &GEnvironmentData;
}

Material::Material()
	: mRenderCore(std::make_shared<RenderCorePBR>())
{

}
Material::Material(const std::string& configFilename)
	: mRenderCore(std::make_shared<RenderCorePBR>())
{
}

bool Material::SetFloat(const std::string& name, std::uint32_t offset, std::uint32_t count, const float* data)
{
	auto it = mParameters.find(name);
	if (it == mParameters.end())
	{
		auto tp = mParameters.emplace(name, MaterialParameter());
		it		= tp.first;
	}

	it->second.Data.resize(sizeof(float) * count);
	memcpy(it->second.Data.data(), data, sizeof(float) * count);

	return true;
}

bool Material::SetTexture(const std::string& name, TexturePtr texture)
{
	mTextures[name] = texture;
	return true;
}

ShadingBuffer Material::GetShadingBuffer() const
{
	ShadingBuffer cShadingBuffer;
	{
		cShadingBuffer.baseColorFactor = Vector4f(1.0f);
		cShadingBuffer.emissiveFactor  = Vector4f(0.0f);
		cShadingBuffer.metallicFactor  = 0.0f;
		cShadingBuffer.roughnessFactor = 0.5f;
	}
	return cShadingBuffer;
}

float Material::GetFloat(const std::string& name) const
{
	auto it = mParameters.find(name);
	if (it == mParameters.end())
	{
		return { 0 };
	}
	return ((float*)it->second.Data.data())[0];
}
vec2 Material::GetVec2(const std::string& name) const
{
	auto it = mParameters.find(name);
	if (it == mParameters.end())
	{
		return { 0, 0 };
	}
	if (it->second.Data.size() <8)
	{
		return vec2(GetFloat(name), 0.0f);
	}
	return ((vec2*)it->second.Data.data())[0];
}
vec3 Material::GetVec3(const std::string& name) const
{
	auto it = mParameters.find(name);
	if (it == mParameters.end())
	{
		return { 0, 0, 0 };
	}
	if (it->second.Data.size() < 12)
	{
		return vec3(GetVec2(name), 0.0f);
	}
	return ((vec3*)it->second.Data.data())[0];
}
vec4 Material::GetVec4(const std::string& name) const
{
	auto it = mParameters.find(name);
	if (it == mParameters.end())
	{
		return { 0, 0, 0, 0 };
	}
	if (it->second.Data.size() < 16)
	{
		return vec4(GetVec3(name), 0.0f);
	}
	return ((vec4*)it->second.Data.data())[0];
}

VertexOutputData RenderCore::InterpolateAttributes(vec2 barycenter, const Geometry* mesh, int primId) noexcept
{
	VertexOutputData OutputData;

	const float u = barycenter.x;
	const float v = barycenter.y;
	const float s = 1 - u - v;

	uint32 v0, v1, v2;
	std::tie(v0, v1, v2) = mesh->GetIndexBuffer()->GetVerticesByPrimitiveId(primId);

	Vector3f p0, p1, p2;
	std::tie(p0, p1, p2) = mesh->GetTripleAttributesByIndex<Vector3f>(VertexBufferAttriKind::NORMAL, v0, v1, v2);

	OutputData.Position = u * p0 + v * p1 + s * p2;

	if (mesh->HasAttribute(VertexBufferAttriKind::NORMAL))
	{
		Vector3f n0, n1, n2;
		std::tie(n0, n1, n2) = mesh->GetTripleAttributesByIndex<Vector3f>(VertexBufferAttriKind::NORMAL, v0, v1, v2);
		n0					 = glm::normalize(n0);
		n1					 = glm::normalize(n1);
		n2					 = glm::normalize(n2);

		OutputData.Normal	 = glm::normalize(u * n0 + v * n1 + s * n2);
		OutputData.HasNormal = true;
	}

	if (mesh->HasAttribute(VertexBufferAttriKind::TEXCOORD))
	{
		Vector2f uv0, uv1, uv2;
		std::tie(uv0, uv1, uv2) = mesh->GetTripleAttributesByIndex<Vector2f>(VertexBufferAttriKind::TEXCOORD, v0, v1, v2);

		OutputData.Texcoord0   = u * uv0 + v * uv1 + s * uv2;
		OutputData.HasTexcoord = true;
	}

	if (mesh->HasAttribute(VertexBufferAttriKind::TANGENT))
	{
		Vector3f n0, n1, n2;
		std::tie(n0, n1, n2) = mesh->GetTripleAttributesByIndex<Vector3f>(VertexBufferAttriKind::TANGENT, v0, v1, v2);
		n0					 = glm::normalize(n0);
		n1					 = glm::normalize(n1);
		n2					 = glm::normalize(n2);

		OutputData.Tangent	  = glm::normalize(u * n0 + v * n1 + s * n2);
		OutputData.HasTangent = true;
	}

	if (mesh->HasAttribute(VertexBufferAttriKind::BITANGENT))
	{
		Vector3f n0, n1, n2;
		std::tie(n0, n1, n2) = mesh->GetTripleAttributesByIndex<Vector3f>(VertexBufferAttriKind::BITANGENT, v0, v1, v2);
		n0					 = glm::normalize(n0);
		n1					 = glm::normalize(n1);
		n2					 = glm::normalize(n2);

		OutputData.BiTangent	= glm::normalize(u * n0 + v * n1 + s * n2);
		OutputData.HasBiTangent = true;
	}

	return OutputData;
}

Color4f RenderCorePBR::Execute(const GlobalConstantBuffer& cGlobalBuffer,
							   const BatchBuffer&		   cBatchBufferconst,
							   const VertexOutputData&	   vertexData,
							   class Material*			   material) noexcept
{
	auto abledoTexture	  = material->GetTexture("tAlbedo");
	auto matTexture		  = material->GetTexture("tMetallicRoughnessMap");
	auto normalTexture	  = material->GetTexture("tNormalMap");
	auto emissiveTexture  = material->GetTexture("tEmissiveMap");
	auto occlusionTexture = material->GetTexture("tOcclusionMap");

	// Get from Material
	ShadingBuffer cShadingBuffer = material->GetShadingBuffer();

	static RxSampler* sampler = RxSampler::CreateSampler(RxSamplerType::Linear, RxWrapMode::Repeat);
	Color3f			  EmissiveColor(0.0f);
	Color3f			  AOColor(0.0f);
	Color3f			  Albedo(cShadingBuffer.baseColorFactor);

	Assert(vertexData.HasNormal);
	Assert(vertexData.HasTexcoord);

	vec2 uv		   = vertexData.Texcoord0;
	vec3 vertexPos = vertexData.Position;

	if (abledoTexture)
	{
		Albedo = Albedo * sampler->ReadPixel(abledoTexture.get(), uv.x, uv.y).xyz;
	}
	else 

	if (emissiveTexture)
	{
		EmissiveColor = sampler->ReadPixel(emissiveTexture.get(), uv.x, uv.y).xyz;
	}
	else 
	{
		EmissiveColor = material->GetVec4("emissiveFactor");
	}

	if (occlusionTexture)
	{
		AOColor = sampler->ReadPixel(occlusionTexture.get(), uv.x, uv.y).xyz;
	}

	Vector3f sunDir	 = -1.0f * Vector3f{ cGlobalBuffer.SunLight.x, cGlobalBuffer.SunLight.y, cGlobalBuffer.SunLight.z };
	sunDir			 = glm::normalize(sunDir);
	Vector3f viewDir = Vector3f(cGlobalBuffer.EyePos) - vertexPos;
	viewDir			 = glm::normalize(viewDir);

	vec3 N;
	if (normalTexture)
	{
		glm::mat3 normalMatrix = glm::mat3(1);
		if (vertexData.HasTangent && vertexData.HasBiTangent)
		{
			const Vector3f& T = vertexData.Tangent;
			const Vector3f& B = vertexData.BiTangent;

			glm::mat3 tbnMatrix	   = glm::mat3(T, B, vertexData.Normal);
			glm::mat3 invTBNMatrix = glm::inverse(tbnMatrix);
			normalMatrix		   = glm::transpose(invTBNMatrix);
		}

		vec3 tn = vec3(sampler->ReadPixel(normalTexture.get(), uv.x, uv.y));
		N		= glm::normalize(normalMatrix * (tn * 2.0f - 1.0f));
	}
	else
	{
		N = glm::normalize(vertexData.Normal);
	}

	// 计算视线方向Metallic
	vec3 V = glm::normalize(viewDir);

	// 计算光线方向和半角向量
	vec3  L = sunDir;
	vec3  H = normalize(L + V);
	vec3  R = reflect(-V, N);

	float roughness;
	float metallic;

	if (matTexture)
	{
		vec4  mat11		= sampler->ReadPixel(matTexture.get(), uv.x, uv.y);
		roughness = mat11.g;
		metallic	= mat11.b;
	}
	else 
	{
		roughness = material->GetFloat("roughnessFactor");
		metallic  = material->GetFloat("metallicFactor");
	}
	


	GBufferData gBufferData{
		.Albedo		   = Albedo,
		.WorldNormal   = N,
		.Position	   = vertexPos,
		.Material	   = vec3(metallic, roughness, 0),
		.EmissiveColor = EmissiveColor,
		.AOMask		   = AOColor,
	};

	return Shading(cGlobalBuffer, gBufferData, *GetEnvironmentData());
}

Color4f RenderCorePBR::Shading(const GlobalConstantBuffer& cGlobalBuffer, const GBufferData& gBufferData, const EnvironmentTextures& gEnvironmentData) const noexcept
{
	vec3  N			= normalize(gBufferData.WorldNormal);
	vec3  V			= normalize(cGlobalBuffer.EyePos.xyz - gBufferData.Position);
	vec3  R			= reflect(-V, N);
	vec3  albedo	= pow(gBufferData.Albedo, vec3(2.2f));
	float metallic	= gBufferData.Material.r;
	float roughness = gBufferData.Material.g;
	vec3  ao		= gBufferData.AOMask;
	vec4  FragColor(0, 0, 0, 1);

	// calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
	// of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
	vec3 F0 = vec3(0.04);
	F0		= mix(F0, albedo, metallic);

	vec3 WorldPos = gBufferData.Position;

	// reflectance equation
	vec3 Lo = vec3(0.0);
	// for (int i = 0; i < 4; ++i)
	for (const auto& light : cGlobalBuffer.Lights)
	{
		vec3 lightPos	= light.Position;
		vec3 lightColor = light.Color * 2.0f;
		// calculate per-light radiance
		vec3  L			  = normalize(lightPos - WorldPos);
		vec3  H			  = normalize(V + L);
		float distance	  = length(lightPos - WorldPos);
		float attenuation = 1.0 / (distance * distance);
		vec3  radiance	  = lightColor * attenuation;

		// Cook-Torrance BRDF
		float NDF = D_GGX(N, H, roughness);
		float G	  = GeometrySmith(N, V, L, roughness);
		vec3  F	  = F_Schlick(clamp(dot(H, V), 0.0f, 1.0f), F0);

		vec3  numerator	  = NDF * G * F;
		float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.0001f; // + 0.0001 to prevent divide by zero
		vec3  specular	  = numerator / denominator;

		// kS is equal to Fresnel
		vec3 kS = F;
		// for energy conservation, the diffuse and specular light can't
		// be above 1.0 (unless the surface emits light); to preserve this
		// relationship the diffuse component (kD) should equal 1.0 - kS.
		vec3 kD = vec3(1.0) - kS;
		// multiply kD by the inverse metalness such that only non-metals
		// have diffuse lighting, or a linear blend if partly metal (pure metals
		// have no diffuse light).
		kD *= 1.0 - metallic;

		// scale light by NdotL
		float NdotL = max(dot(N, L), 0.0f);

		// add to outgoing radiance Lo
		Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
	}

	// ambient lighting (we now use IBL as the ambient term)
	vec3 F = F_SchlickR(max(dot(N, V), 0.0f), F0, roughness);

	vec3 kS = F;
	vec3 kD = 1.0f - kS;
	kD *= 1.0f - metallic;


	vec3 irradiance = textureCubeLod(gEnvironmentData.IrradianceTexture, WorldPos, 0).rgb;
	vec3 diffuse	= irradiance * albedo;

	//// sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
	const float MAX_REFLECTION_LOD = 4.0;
	vec3		prefilteredColor   = textureCubeLod(gEnvironmentData.EnvTexture, glm::normalize(R), int(roughness * MAX_REFLECTION_LOD)).rgb;
	vec2		brdf			   = texture2D(gEnvironmentData.BRDFTexture, vec2(max(dot(N, V), 0.0f), roughness)).rg;
	vec3		specular		   = prefilteredColor * (F * brdf.x + brdf.y);

	vec3 ambient = (kD * diffuse + specular + gBufferData.EmissiveColor) * ao;

	vec3 color = ambient + Lo;

	// HDR tonemapping
	color = color / (color + vec3(1.0));
	// gamma correct
	color = pow(color, vec3(1.0 / 2.2));

	return vec4(color, 1.0);
}
