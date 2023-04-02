
#include "PBRRender.h"
#include "Graphics/RxSampler.h"
#include <algorithm>

#define USE_ALITA_PBR 0

#define PI 3.1415926535897932384626433832795f
#define Epsilon  0.00001f

template<class T>
constexpr T pow2(T x)
{
	return x * x;
}

template <typename T1, typename T2>
T1 mix(T1 a, T1 b, T2 c)
{
	return glm::mix(a, b, c);
}

template<typename T1, typename T2>
T1 clamp(T1 a, T2 b, T2 c)
{
	return glm::clamp(a, b, c);
}

template<typename T1>
T1 max(T1 a, T1 b)
{
	return glm::max(a, b);
}

template<class T>
constexpr size_t nextPowerOfTwo(T i)
{
	// Equivalent to 2^ceil(log2(i))
	return i == 0 ? 0 : 1ull << (sizeof(T) * 8 - std::countl_zero(i - 1));
}

float determinant(const TMat3x3& M)
{
	return {
		M[0][0] * (M[1][1] * M[2][2] - M[2][1] * M[1][2]) -
		M[1][0] * (M[0][1] * M[2][2] - M[2][1] * M[0][2]) +
		M[2][0] * (M[0][1] * M[1][2] - M[1][1] * M[0][2])
	};
}

TMat3x3 inverse(const TMat3x3& M)
{
	float inv_det = 1.0 / determinant(M);

	return {
		{ inv_det * (M[1][1] * M[2][2] - M[2][1] * M[1][2]),
		  inv_det * (M[2][1] * M[0][2] - M[0][1] * M[2][2]),
		  inv_det * (M[0][1] * M[1][2] - M[1][1] * M[0][2]) },
		{ inv_det * (M[2][0] * M[1][2] - M[1][0] * M[2][2]),
		  inv_det * (M[0][0] * M[2][2] - M[2][0] * M[0][2]),
		  inv_det * (M[1][0] * M[0][2] - M[0][0] * M[1][2]) },
		{ inv_det * (M[1][0] * M[2][1] - M[2][0] * M[1][1]),
		  inv_det * (M[2][0] * M[0][1] - M[0][0] * M[2][1]),
		  inv_det * (M[0][0] * M[1][1] - M[1][0] * M[0][1]) }
	};
}

Vector3f mult(const TMat3x3& M, const Vector3f v)
{
	return {
		M[0][0] * v[0] + M[1][0] * v[1] + M[2][0] * v[2],
		M[0][1] * v[0] + M[1][1] * v[1] + M[2][1] * v[2],
		M[0][2] * v[0] + M[1][2] * v[1] + M[2][2] * v[2]
	};
}

Vector3f mult(const Vector3f& v, float a)
{
	return { v[0] * a, v[1] * a, v[2] * a };
}


float cfloor(float x)
{
	return static_cast<float>(static_cast<intmax_t>(x));
}

template<class T>
constexpr T cmax(T x, T y)
{
	return x > y ? x : y;
}

typedef Vector2f vec2;
typedef Vector3f vec3;
typedef Vector4f vec4;

namespace C
{
	inline constexpr float INV_PI  = 0.31830988618379067154;
	inline constexpr float HALF_PI = 1.57079632679489661923;
	inline constexpr float TWO_PI  = 6.283185307179586476925;
	inline constexpr float EPSILON = 1e-9;
} // namespace C

/*

- Sampling the GGX Distribution of Visible Normals
  http://jcgt.org/published/0007/04/01/

- Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs
  http://jcgt.org/published/0003/02/03/

- Microfacet Models for Refraction through Rough Surfaces
  https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.html

*/

float GGX::D(const Vector3f& m, const Vector2f& a)
{
	return 1.0 / (PI * a.x * a.y * pow2(pow2(m.x / a.x) + pow2(m.y / a.y) + pow2(m.z)));
}

float GGX::DV(const Vector3f& m, const Vector3f& wo, const Vector2f& a)
{
	return SmithG1(wo, a) * glm::dot(wo, m) * D(m, a) / wo.z;
}

float GGX::Lambda(const Vector3f& wo, const Vector2f& a)
{
	return (-1.0 + std::sqrt(1.0 + (pow2(a.x * wo.x) + pow2(a.y * wo.y)) / (pow2(wo.z)))) / 2.0;
}

float GGX::SmithG1(const Vector3f& wo, const Vector2f& a)
{
	return 1.0 / (1.0 + Lambda(wo, a));
}

float GGX::SmithG2(const Vector3f& wi, const Vector3f& wo, const Vector2f& a)
{
	return 1.0 / (1.0 + Lambda(wo, a) + Lambda(wi, a));
}

float GGX::reflection(const Vector3f& wi, const Vector3f& wo, const Vector2f& a, float& PDF)
{
	Vector3f m = glm::normalize(wo + wi);

	PDF = DV(m, wo, a) / (4.0 * glm::dot(m, wo));
	return D(m, a) * SmithG2(wi, wo, a) / (4.0 * wo.z * wi.z);
}

float GGX::transmission(const Vector3f& wi, const Vector3f& wo, float n1, float n2, const Vector2f& a, float& PDF)
{
	Vector3f m		   = wo * n1 + wi * n2;
	float	 m_length2 = glm::dot(m, m);
	m /= std::sqrt(m_length2);
	if (n1 < n2)
		m = -m;

	float dm_dwi = pow2(n2) * std::abs(glm::dot(wi, m)) / m_length2;

	PDF = DV(m, wo, a) * dm_dwi;
	return std::abs(SmithG2(wi, wo, a) * D(m, a) * glm::dot(wo, m) * dm_dwi / (wo.z * wi.z));
}

Vector3f GGX::visibleMicrofacet(float u, float v, const Vector3f& wo, const Vector2f& a)
{
	Vector3f Vh = glm::normalize(Vector3f(a.x * wo.x, a.y * wo.y, wo.z));

	// Section 4.1: orthonormal basis (with special case if cross product is zero)
	float	 len2 = pow2(Vh.x) + pow2(Vh.y);
	Vector3f T1	  = len2 > 0.0 ? Vector3f(-Vh.y, Vh.x, 0.0f) * glm::inversesqrt(len2) : Vector3f(1.0, 0.0, 0.0);
	Vector3f T2	  = glm::cross(Vh, T1);

	// Section 4.2: parameterization of the projected area
	float r	  = std::sqrt(u);
	float phi = v * C::TWO_PI;
	float t1  = r * std::cos(phi);
	float t2  = r * std::sin(phi);
	float s	  = 0.5 * (1.0 + Vh.z);
	t2		  = (1.0 - s) * std::sqrt(1.0 - pow2(t1)) + s * t2;

	// Section 4.3: reprojection onto hemisphere
	Vector3f Nh = t1 * T1 + t2 * T2 + std::sqrt(std::max(0.0f, 1.0f - pow2(t1) - pow2(t2))) * Vh;

	// Section 3.4: transforming the normal back to the ellipsoid configuration
	return glm::normalize(Vector3f(a.x * Nh.x, a.y * Nh.y, std::max(0.0f, Nh.z)));
}


// 定义GGX函数
float _GGX(float NdotH, float a)
{
	float a2 = a * a;
	float d	 = (NdotH * a2 - NdotH) * NdotH + 1.0;
	return a2 / (M_PI * d * d);
}


// 定义常量
const vec3	LightColor		  = vec3(1.0, 1.0, 1.0);	// 光源颜色
const vec3	AmbientLightColor = vec3(1.0, 1.0, 1.0);	// 环境光颜色
const vec3	F0				  = vec3(0.04);				// 表面的反射率
//const vec3	LightFalloff	  = vec3(1.0, 0.09, 0.032); // 光照衰减
const vec3	LightFalloff	  = vec3(0.5); // 光照衰减
const float u_AmbientLightIntensity = 0.1f;


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
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (vec3(1.0f) - F0) * glm::pow(1.0f - cosTheta, 5.0f);
}

float geometrySchlickGGX(float NdotV, float k)
{
	float a = k / 2.0;
	float b = 1.0 - a;
	return NdotV / (NdotV * b + a);
}

float geometrySmith(float NdotV, float NdotL, float k)
{
	return 2.0 * NdotL * NdotV / (NdotV * (1.0 - k) + k) + 2.0 * NdotL * (1.0 - NdotV);
}

float GGX2(float NdotH, float roughness)
{
	float a	 = roughness * roughness;
	float a2 = a * a;
	float d	 = (NdotH * a2 - NdotH) * NdotH + 1.0;
	return a2 / (PI * d * d);
}

vec3 PBR(vec3 albedo, float roughness, float metallic, vec3 N, vec3 V, vec3 L, vec3 H)
{
	vec3  F0	   = mix(vec3(0.04f), albedo, metallic);
	vec3  F		   = fresnelSchlick(glm::max(dot(H, V), 0.0f), F0);
	float NDF	   = GGX2(glm::max(dot(N, H), 0.0f), roughness);
	float G		   = geometrySchlickGGX(glm::max(dot(N, V), 0.0f), roughness) * geometrySchlickGGX(glm::max(glm::dot(N, L), 0.0f), roughness);
	vec3  diffuse  = (1.0f - F) * albedo * (1.0f - metallic);
	vec3  specular = F * NDF * G * metallic;
	return (diffuse + specular) * LightColor / pow(distance(L, H), 2.0f) ;//* LightFalloff;
}


//#define ALBEDO texture(sampler2D(tGDiffuse, sGDiffuse), _UV0).rgb

// From http://filmicgames.com/archives/75
vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
	float alpha	 = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom	 = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2) / (PI * denom * denom);
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r	 = (roughness + 1.0);
	float k	 = (r * r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Fresnel function ----------------------------------------------------
vec3 F_Schlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}
vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0f - cosTheta, 5.0f);
}

vec3 prefilteredReflection(vec3 R, float roughness)
{
	const float MAX_REFLECTION_LOD = 9.0; // todo: param/const
	float		lod				   = roughness * MAX_REFLECTION_LOD;
	float		lodf			   = floor(lod);
	float		lodc			   = ceil(lod);
	vec3		a				   = vec3{ 0.1f };
	vec3		b				   = vec3{ 0.1f };
	/*vec3		a				   = textureLod(samplerCube(tCubeMap, sCubeMap), R, lodf).rgb;
	vec3		b				   = textureLod(samplerCube(tCubeMap, sCubeMap), R, lodc).rgb;*/

	// vec3 a = textureLod(prefilteredMap, R, lodf).rgb;
	// vec3 b = textureLod(prefilteredMap, R, lodc).rgb;
	return mix(a, b, lod - lodf);
}

vec3 specularContribution(vec3 L, vec3 V, vec3 N, vec3 F0, vec3 albedo, float metallic, float roughness)
{
	// Precalculate vectors and dot products
	vec3  H		= glm::normalize(V + L);
	float dotNH = glm::clamp(dot(N, H), 0.0f, 1.0f);
	float dotNV = glm::clamp(dot(N, V), 0.0f, 1.0f);
	float dotNL = glm::clamp(dot(N, L), 0.0f, 1.0f);

	// Light color fixed
	vec3 lightColor = vec3(2.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0)
	{
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, roughness);
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		vec3 F	  = F_Schlick(dotNV, F0);
		vec3 spec = D * F * G / (4.0f * dotNL * dotNV + 0.001f);
		vec3 kD	  = (vec3(1.0f) - F) * (1.0f - metallic);
		color += (kD * albedo / PI + spec) * dotNL;
	}

	return color;
}

// Specular BRDF composition --------------------------------------------

// Fresnel function ----------------------------------------------------
vec3 F_Schlick(float cosTheta, float metallic, vec3 albedo)
{
	vec3 F0 = mix(vec3(0.04f), albedo, metallic); // * material.specular
	vec3 F	= F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
	return F;
}

vec3 BRDF(vec3 L, vec3 V, vec3 N, vec3 albedo, float metallic, float roughness)
{
	// Precalculate vectors and dot products
	vec3  H		= normalize(V + L);
	float dotNV = clamp(dot(N, V), 0.0f, 1.0f);
	float dotNL = clamp(dot(N, L), 0.0f, 1.0f);
	float dotLH = clamp(dot(L, H), 0.0f, 1.0f);
	float dotNH = clamp(dot(N, H), 0.0f, 1.0f);

	// Light color fixed
	vec3 lightColor = vec3(1.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0)
	{
		float rroughness = max(0.05f, roughness);
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, roughness);
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		vec3 F = F_Schlick(dotNV, metallic, albedo);

		vec3 spec = D * F * G / (4.0f * dotNL * dotNV);

		color += spec * dotNL * lightColor;
	}

	return color;
}

struct LightData
{
	vec3 direction;
	vec3 radiance;
};

// https://github.com/Nadrin/PBR/blob/master/data/shaders/glsl/pbr_fs.glsl
vec4 PBR2(vec3 Lo, vec3 N, vec3 albedo, float metalness, float roughness, const std::vector<LightData>& lights)
{

	// Constant normal incidence Fresnel factor for all dielectrics.
	const vec3 Fdielectric = vec3(0.04);

	// Angle between surface normal and outgoing light direction.
	float cosLo = max(0.0f, dot(N, Lo));

	// Specular reflection vector.
	vec3 Lr = 2.0f * cosLo * N - Lo;

	// Fresnel reflectance at normal incidence (for metals use albedo color).
	vec3 F0 = mix(Fdielectric, albedo, metalness);

	// Direct lighting calculation for analytical lights.
	vec3 directLighting = vec3(0);
	const int NumLights		 = 1;
	for (int i = 0; i < NumLights; ++i)
	{
		vec3 Li		   = -lights[i].direction;
		vec3 Lradiance = lights[i].radiance;

		// Half-vector between Li and Lo.
		vec3 Lh = normalize(Li + Lo);

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0f, dot(N, Li));
		float cosLh = max(0.0f, dot(N, Lh));

		// Calculate Fresnel term for direct lighting.
		vec3 F = fresnelSchlick(max(0.0f, dot(Lh, Lo)) , F0);
		// Calculate normal distribution for specular BRDF.
		float D = ndfGGX(cosLh, roughness);
		// Calculate geometric attenuation for specular BRDF.
		float G = gaSchlickGGX(cosLi, cosLo, roughness);

		// Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
		// Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
		// To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
		vec3 kd = mix(vec3(1.0f) - F, vec3(0.0f), metalness);

		// Lambert diffuse BRDF.
		// We don't scale by 1/PI for lighting & material units to be more convenient.
		// See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
		vec3 diffuseBRDF = kd * albedo;

		// Cook-Torrance specular microfacet BRDF.
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0f * cosLi * cosLo);

		// Total contribution for this light.
		directLighting += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	}

	// Ambient lighting (IBL).
	vec3 ambientLighting(0.0f);
	//{
	//	// Sample diffuse irradiance at normal direction.
	//	vec3 irradiance = texture(irradianceTexture, N).rgb;

	//	// Calculate Fresnel term for ambient lighting.
	//	// Since we use pre-filtered cubemap(s) and irradiance is coming from many directions
	//	// use cosLo instead of angle with light's half-vector (cosLh above).
	//	// See: https://seblagarde.wordpress.com/2011/08/17/hello-world/
	//	vec3 F = fresnelSchlick(F0, cosLo);

	//	// Get diffuse contribution factor (as with direct lighting).
	//	vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metalness);

	//	// Irradiance map contains exitant radiance assuming Lambertian BRDF, no need to scale by 1/PI here either.
	//	vec3 diffuseIBL = kd * albedo * irradiance;

	//	// Sample pre-filtered specular reflection environment at correct mipmap level.
	//	int	 specularTextureLevels = textureQueryLevels(specularTexture);
	//	vec3 specularIrradiance	   = textureLod(specularTexture, Lr, roughness * specularTextureLevels).rgb;

	//	// Split-sum approximation factors for Cook-Torrance specular BRDF.
	//	vec2 specularBRDF = texture(specularBRDF_LUT, vec2(cosLo, roughness)).rg;

	//	// Total specular IBL contribution.
	//	vec3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;

	//	// Total ambient lighting contribution.
	//	ambientLighting = diffuseIBL + specularIBL;
	//}

	// Final fragment color.
	return vec4(directLighting + ambientLighting, 1.0);
}

Color4f PBRRender::Render(const GlobalConstantBuffer& cGlobalBuffer, const BatchBuffer& cBatchBuffer, const ShadingBuffer& cShadingBuffer, Vector3f pos, Vector3f normal, Vector2f uv, const Material* material) noexcept
{
	auto texture = material->GetTexture("tAlbedo");
	auto matTexture = material->GetTexture("tMetallicRoughnessMap");

	 static RxSampler* sampler = RxSampler::CreateSampler(RxSamplerType::Linear, RxWrapMode::Repeat);
	Color4f			  _albedo = sampler->SamplePixel(texture.get(), uv.x, uv.y);
	Color3f			  Albedo  = { _albedo.r, _albedo.g, _albedo.b };

	return Color4f(Albedo, 1.0f);

	// auto texture	 = material->GetTexture("tNormalMap");
	Vector3f Normal	 = glm::normalize(normal);
	Vector3f sunDir	 = -1.0f * Vector3f{ cGlobalBuffer.SunLight.x, cGlobalBuffer.SunLight.y, cGlobalBuffer.SunLight.z };
	sunDir			 = glm::normalize(sunDir);
	Vector3f viewDir = Vector3f(cGlobalBuffer.EyePos) - pos;
	viewDir			 = glm::normalize(viewDir);

	

	vec3 ALBEDO = Albedo;

	// 计算法线
	vec3 N = glm::normalize(Normal);

	// 计算视线方向Metallic
	vec3 V = glm::normalize(viewDir);

	// 计算光线方向和半角向量
	vec3 L = sunDir;
	vec3 H = normalize(L + V);
	vec3 R = reflect(-V, N); 

	vec4  mat11		= sampler->SamplePixel(matTexture.get(), uv.x, uv.y);
	float metallic	= mat11.b;
	float roughness = mat11.g;

	float Roughness = roughness;
	float Metallic  = metallic;


	//// ----- github glsl
	//std::vector<LightData> lights;
	//lights.emplace_back(sunDir, vec3(1.0f));
	//return PBR2(V, N, Albedo, metallic, roughness, lights);

	///----

	#if USE_ALITA_PBR || 0
#if 1
	vec3 F0 = vec3(0.04);
	F0		= mix(F0, ALBEDO, metallic);

	vec3 Lo = vec3(0.0);
	Lo += specularContribution(L, V, N, F0, ALBEDO, metallic, roughness);

	vec4 brdf4 = sampler->SamplePixel(mBRDFTexture.get(), vec2(max(dot(N, V), 0.0f)));
	vec2 brdf  = vec2(brdf4.r, brdf4.g);
	//vec2 brdf		= texture(sampler2D(tIBLBrdfMap, sIBLBrdfMap), vec2(max(dot(N, V), 0.0), roughness)).rg;
	vec3 reflection = prefilteredReflection(R, roughness);

	//vec3 irradiance = texture(samplerCube(tIrradiance, sIrradiance), N).rgb;
	vec3 irradiance = vec3{ 1.0f };

	// Diffuse based on irradiance
	vec3 diffuse = irradiance * ALBEDO;

	vec3 F = F_SchlickR(max(dot(N, V), 0.0f), F0, roughness);

	// Specular reflectance
	vec3 specular = reflection * (F * brdf.x + brdf.y);

	// Ambient part
	vec3 kD = 1.0f - F;
	kD *= 1.0f - metallic;
	vec3 ambient = (kD * diffuse + specular); // TODO * texture(aoMap, inUV).rrr;

	vec3 color = ambient + Lo;

	//color = vec3(roughness);

	return vec4(color, 1.0f);

#else
	vec3 Lo = vec3(0.0);
	Lo += BRDF(L, V, N, Albedo, metallic, roughness);

	vec3 color = Albedo;
	color += Lo;

	return vec4(ALBEDO, 1.0f);
#endif

#else // USE_ALITA_PBR
	//// 计算环境光
	//vec3 ambient = u_AmbientLightIntensity * AmbientLightColor * Albedo;

	//// 计算PBR光照
	//vec3 color_1 = PBR(Albedo, Roughness, Metallic, N, V, L, H);

	//// 输出最终颜色
	////auto FragColor = vcolor_1ec4{ ambient + color, 1.0f };

	//return vec4{ color_1, 1.0f };

	 // -------------------------------------------1

	Vector3f lightDir = sunDir;

	// 计算半角向量和法线向量
	float NdotH = glm::dot(H, N);

	// 计算几何遮挡因子
	float NdotL			= dot(N, lightDir);
	float NdotV			= dot(N, viewDir);
	float VdotH			= dot(viewDir, H);
	float roughness2	= Roughness * Roughness;
	float G				= 2.0f * NdotH * std::min(NdotV, NdotL) / VdotH;
	float G1			= 2.0f * NdotV / VdotH;
	float G2			= 2.0f * NdotL / VdotH;
	float k				= roughness2 / 2.0;
	float ao			= 1.0f / (1.0f + k * (1.0f - NdotL) * (1.0f - NdotV));
	float roughness4	= roughness2 * roughness2;
	float specular		= _GGX(NdotH, roughness2);
	float F				= 0.5f + 0.5f * pow(1.0f - VdotH, 5.0f);
	vec3  diffuse0		= vec3(1.0f / M_PI) * Albedo;
	vec3  specularF		= vec3(specular);
	vec3  F0			= mix(vec3(0.04f), Albedo, Metallic);
	vec3  kS			= mix(F0, vec3(specularF), Metallic);
	vec3  kD			= vec3(diffuse0) * (1.0f - Metallic);
	vec3  nominator		= kS * G * F;
	float denominator	= 4.0f * NdotL * NdotV;
	vec3  specularColor = nominator / std::max(denominator, 0.001f);
	vec3  ambient0		= vec3(diffuse0) * ao;
	vec3  finalColor	= kD + glm::clamp(specularColor, 0.0f, 1.0f);
	//return vec4(glm::clamp(specularColor, 0.0f, 1.0f), 1.0f);
	vec4  FragColor		= vec4(finalColor + ambient0, 1.0f);

	return FragColor;

	//-----------------------------------

	// auto texture	 = material->GetTexture("tNormalMap");
	 normal			 = glm::normalize(normal);

	// TODO 计算N，L，V，R四个向量并归一化
	 vec3 N_norm = glm::normalize(normal);
	 vec3 L_norm = glm::normalize(sunDir);
	 vec3 V_norm = glm::normalize(viewDir);
	 vec3 R_norm = glm::reflect(-L_norm, N_norm);

	// TODO 计算漫反射系数和镜面反射系数
	 float lambertian = glm::clamp(glm::dot(L_norm, N_norm), 0.0f, 1.0f);
	 float specular0	 = glm::clamp(glm::dot(R_norm, V_norm), 0.0f, 1.0f);

	 vec3 ambiColor = { 0.0f, 0.f, 0.f };

	 Color4f color	  = sampler->SamplePixel(texture.get(), uv.x, uv.y);
	 vec3	diffColor = { color.r, color.g, color.b };
	 vec3	specColor = { cGlobalBuffer.SunLightColor };

	 vec4 ret = vec4(ambiColor + specColor * lambertian + diffColor * pow(specular0, 2.0f), 1.0f);

	// return { color };
	 return { int(255 * ret.r), int(255 * ret.g), int(255 * ret.b), 255 };
#endif // USE_ALITA_PBR
}