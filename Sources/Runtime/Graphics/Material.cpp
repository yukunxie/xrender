#include "Material.h"
#include "Meshes/Geometry.h"
#include "VertexBuffer.h"
#include "Graphics/RxSampler.h"
#include "PathTracer/Raytracer.h"

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

template<typename T>
T Interpolate3Points(bool isCCW, vec2 barycenter, const T& p0, const T& p1, const T& p2)
{
	const float u = barycenter.x;
	const float v = barycenter.y;
	const float w = 1.0f - u - v;
	return w * p0 + u * p1 + v * p2;
}

VertexOutputData RenderCore::InterpolateAttributes(vec2 barycenter, vec3 ng, const TMat4x4& worldMatrix, const Geometry* mesh, int primId) noexcept
{
	VertexOutputData OutputData;

	uint32 v0, v1, v2;
	std::tie(v0, v1, v2) = mesh->GetIndexBuffer()->GetVerticesByPrimitiveId(primId);

	Vector3f p0, p1, p2;
	mesh->GetTripleAttributesByIndex<Vector3f>(VertexBufferAttriKind::POSITION, v0, v1, v2, p0, p1, p2);
	

	bool isCCW = glm::dot(glm::cross(p1 - p0, p2 - p0), -ng) > 0;
	
	OutputData.Position = Interpolate3Points(isCCW, barycenter, p0, p1, p2);

	

	OutputData.Position = (worldMatrix * vec4(OutputData.Position, 1.0f)).xyz;

	Vector2f uv0, uv1, uv2;

	if (mesh->HasAttribute(VertexBufferAttriKind::TEXCOORD))
	{

		mesh->GetTripleAttributesByIndex<Vector2f>(VertexBufferAttriKind::TEXCOORD, v0, v1, v2, uv0, uv1, uv2);

		OutputData.Texcoord0   = Interpolate3Points(isCCW, barycenter, uv0, uv1, uv2);
		OutputData.HasTexcoord = true;
	}

	if (mesh->HasAttribute(VertexBufferAttriKind::NORMAL))
	{
		Vector3f n0, n1, n2;
		mesh->GetTripleAttributesByIndex<Vector3f>(VertexBufferAttriKind::NORMAL, v0, v1, v2, n0, n1, n2);
		n0					 = glm::normalize(n0);
		n1					 = glm::normalize(n1);
		n2					 = glm::normalize(n2);
		Vector3f normal		 = Interpolate3Points(isCCW, barycenter, n0, n1, n2);

		OutputData.Normal	 = glm::normalize(normal);
		OutputData.HasNormal = true;
	}
	if (mesh->HasAttribute(VertexBufferAttriKind::TANGENT))
	{
		Vector3f t0, t1, t2;
		mesh->GetTripleAttributesByIndex<Vector3f>(VertexBufferAttriKind::TANGENT, v0, v1, v2, t0, t1, t2);
		t0					 = glm::normalize(t0);
		t1					 = glm::normalize(t1);
		t2					 = glm::normalize(t2);

		vec3 T = glm::normalize(Interpolate3Points(isCCW, barycenter, t0, t1, t2));
		T					  = glm::normalize(T - glm::dot(T, OutputData.Normal) * OutputData.Normal); // 计算切线在法线平面上的投影
		
		OutputData.HasTangent = true;
		OutputData.Tangent	  = T;

		OutputData.BiTangent = glm::cross(OutputData.Normal, T); // 计算副切线
		OutputData.HasBiTangent = true;
	}
	
	return OutputData;
}

RenderOutputData RenderCorePBR::Execute(const Raytracer*			rayTracer,
							   const GlobalConstantBuffer& cGlobalBuffer,
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
	Color3f			  AOColor(1.0f);
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
	{
		Albedo = material->GetVec4("baseColorFactor");
	}

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
			float k = glm::dot(vertexData.Tangent, vertexData.Normal);
			//Assert( < 0.001f);
			const Vector3f& T = vertexData.Tangent;
			const Vector3f& B = vertexData.BiTangent;

			glm::mat3 tbnMatrix	   = glm::mat3(T, B, vertexData.Normal);
			glm::mat3 invTBNMatrix = glm::inverse(tbnMatrix);
			normalMatrix		   = glm::transpose(invTBNMatrix);
		}
		else
		{
			Assert(false);
		}

		vec3 tn = vec3(sampler->ReadPixel(normalTexture.get(), uv.x, uv.y));
		N		= glm::normalize(normalMatrix * glm::normalize((tn * 2.0f - 1.0f)));
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

	return Shading(rayTracer, cGlobalBuffer, gBufferData, *GetEnvironmentData());
}

vec3 CalculateLight(vec3 albedo, vec3 radiance, vec3 N, vec3 V, vec3 L, vec3 F0, float metallic, float roughness)
{
	vec3 H = normalize(V + L);

	N = glm::abs(N);
	// Cook-Torrance BRDF
	float NDF = D_GGX(N, H, roughness);
	float G	  = GeometrySmith(N, V, L, roughness);
	vec3  F	  = F_Schlick(clamp(dot(H, V), 0.0f, 1.0f), F0);

	vec3  numerator	  = NDF * G * F;
	float NdotV		  = dot(N, V);
	float NdotL		  = dot(N, L);
	float denominator = 4.0f * max(NdotV, 0.0f) * max(NdotL, 0.0f) + 0.0001f; // + 0.0001 to prevent divide by zero
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
	//float NdotL = max(dot(N, L), 0.0f);

	// add to outgoing radiance Lo
	return (kD * albedo / PI + specular) * radiance * max(dot(N, L), 0.0f); // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
}

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float ndfGGX(float cosLh, float roughness)
{
	float alpha	  = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX(float cosLi, float cosLo, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

// Shlick's approximation of the Fresnel factor.
vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (vec3(1.0f) - F0) * pow(1.0f - cosTheta, 5.0f);
}

vec3 SunLightDirectLignting(vec3 albedo, vec3 normal, vec3 viewDir, vec3 sunDir, vec3 sunColor, float metallic, float roughness)
{

	vec3 Li		   = sunDir;
	vec3 Lradiance = sunColor;
	vec3 Lo		   = viewDir;
	vec3 N		   = normal;

	//vec3 albedo = vec3(1.0f);

	// Half-vector between Li and Lo.
	vec3 Lh = normalize(Li + Lo);

	// Calculate angles between surface normal and various light vectors.
	float cosLi = max(0.0f, dot(N, Li));
	float cosLh = max(0.0f, dot(N, Lh));

	// Angle between surface normal and outgoing light direction.
	float cosLo = max(0.0f, dot(N, Lo));

	// Specular reflection vector.
	vec3 Lr = 2.0f * cosLo * N - Lo;
	const vec3 Fdielectric = vec3(0.04);

	// Fresnel reflectance at normal incidence (for metals use albedo color).
	vec3 F0 = mix(Fdielectric, albedo, metallic);

	// Calculate Fresnel term for direct lighting.
	vec3 F = fresnelSchlick(F0, max(0.0f, dot(Lh, Lo)));
	// Calculate normal distribution for specular BRDF.
	float D = ndfGGX(cosLh, roughness);
	// Calculate geometric attenuation for specular BRDF.
	float G = gaSchlickGGX(cosLi, cosLo, roughness);

	// Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
	// Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
	// To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
	vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metallic);

	// Lambert diffuse BRDF.
	// We don't scale by 1/PI for lighting & material units to be more convenient.
	// See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
	vec3 diffuseBRDF = kd * albedo;

	// Cook-Torrance specular microfacet BRDF.
	vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0f * cosLi * cosLo);

	// Total contribution for this light.
	return (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
}

RenderOutputData RenderCorePBR::Shading(const Raytracer*			rayTracer,
							   const GlobalConstantBuffer& cGlobalBuffer,
							   const GBufferData&		   gBufferData,
							   const EnvironmentTextures&  gEnvironmentData) const noexcept
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
		vec3 lightColor = light.Color;
		// calculate per-light radiance
		vec3  L			  = normalize(lightPos - WorldPos);
		vec3  H			  = normalize(V + L);
		float distance	  = length(lightPos - WorldPos);
		/*if (distance > 300)
			continue;*/
		float attenuation = 1.0 / (distance * distance);
		vec3  radiance	  = lightColor * attenuation;

		if (!rayTracer->IsShadowRay(gBufferData.Position, lightPos))
		{
			Lo += glm::max(vec3(0), CalculateLight(albedo, radiance, N, V, L, F0, metallic, roughness));
		}
	}

	// Add directional light
	{
		vec3 sunDir	  = -1.0f * glm::normalize(cGlobalBuffer.SunLight);
		vec3 sunColor = cGlobalBuffer.SunLightColor.xyz;

		vec3 to = gBufferData.Position + sunDir * 50.0f;
		 if (!rayTracer->IsShadowRay(gBufferData.Position, to))
		{
			Lo += SunLightDirectLignting(albedo, N, V, sunDir, sunColor, metallic, roughness);
		}
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
	//color = color / (color + vec3(1.0));
	color = ACESToneMapping(color /* * 4.5f*/, 1.0f);
	// gamma correct
	color = pow(color, vec3(1.0 / 2.2));

	RenderOutputData output;
	{
		output.Color		 = vec4(color, 1.0);
		output.WorldPosition = WorldPos;
		output.Normal		 = N;
	}	

	// calc depth
	{
		auto ViewMatrix = cGlobalBuffer.ViewMatrix;
		auto ProjMatrix = cGlobalBuffer.ProjMatrix;
		auto mvp		= ProjMatrix * ViewMatrix;
		vec4 np			= mvp * vec4(WorldPos, 1.0f);
		np /= np.w;
		np.xyz = np.xyz * 0.5f + 0.5f;
		output.Depth = np.z;
	}

	return output;
}

RenderOutputData RenderCoreSkybox::Execute(const Raytracer*			   rayTracer,
								  const GlobalConstantBuffer& cGlobalBuffer,
								  const VertexOutputData&	  vertexData,
								  class Material*			  material) noexcept
{
	auto	  evnTexture = GetEnvironmentData()->EnvTexture;
	glm::vec4 viewPos	 = cGlobalBuffer.ViewMatrix * glm::vec4(vertexData.Position, 1.0f);
	glm::vec3 viewCoords = viewPos.xyz;
	viewCoords			 = viewCoords / viewPos.w;

	glm::mat4 skyboxViewMatrix = glm::mat4(glm::mat3(cGlobalBuffer.ViewMatrix));
	glm::vec3 skyboxCoords	   = glm::vec3(skyboxViewMatrix * glm::vec4(viewCoords, 1.0f));

	vec3 color = textureCubeLod(evnTexture, vertexData.Position.xyz, 0).rgb;
	auto FragColor = vec4(color, 1.0);

	RenderOutputData output;
	{
		output.Color = FragColor;
		output.WorldPosition = vertexData.Position;
		output.Normal = vertexData.Normal;
	}

	// calc depth
	{
		auto ViewMatrix = cGlobalBuffer.ViewMatrix;
		auto ProjMatrix = cGlobalBuffer.ProjMatrix;
		auto mvp		= ProjMatrix * ViewMatrix;
		vec4 np			= mvp * vec4(vertexData.Position, 1.0f);
		np /= np.w;
		np.xyz		 = np.xyz * 0.5f + 0.5f;

		output.Depth = np.z;
	}

	return output;
}


RenderOutputData RenderCoreUnlit::Execute(const Raytracer*			  rayTracer,
							   const GlobalConstantBuffer& cGlobalBuffer,
							   const VertexOutputData&	   vertexData,
							   class Material*			   material) noexcept
{
	auto abledoTexture = material->GetTexture("tAlbedo");
	vec2 uv			   = vertexData.Texcoord0;
	// Get from Material
	ShadingBuffer cShadingBuffer = material->GetShadingBuffer();

	static RxSampler* sampler = RxSampler::CreateSampler(RxSamplerType::Linear, RxWrapMode::Repeat);

	RenderOutputData output;

	if (abledoTexture)
	{
		output.Color = sampler->ReadPixel(abledoTexture.get(), uv.x, uv.y);
	}
	else if (material->HasParameter("baseColorFactor"))
	{
		output.Color =  material->GetVec4("baseColorFactor");
	}
	else
	{
		output.Color =  vec4(1.0f);
	}
	output.WorldPosition = vertexData.Position;
	output.Normal = vertexData.Normal;

	// calc depth
	{
		auto ViewMatrix = cGlobalBuffer.ViewMatrix;
		auto ProjMatrix = cGlobalBuffer.ProjMatrix;
		auto mvp		= ProjMatrix * ViewMatrix;
		vec4 np			= mvp * vec4(vertexData.Position, 1.0f);
		np /= np.w;
		np.xyz = np.xyz * 0.5f + 0.5f;

		output.Depth = np.z;
	}

	return output;
}