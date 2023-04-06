#include "PBRShader.h"
#include "Graphics/RxSampler.h"

#include <algorithm>


#define PI		3.1415926535897932384626433832795f
#define Epsilon 0.00001f

#define COOK_GGX

template<class T>
constexpr T pow2(T x)
{
	return x * x;
}

template<typename T1, typename T2>
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

template<typename T1>
T1 min(T1 a, T1 b)
{
	return glm::min(a, b);
}

template<class T>
constexpr size_t nextPowerOfTwo(T i)
{
	// Equivalent to 2^ceil(log2(i))
	return i == 0 ? 0 : 1ull << (sizeof(T) * 8 - std::countl_zero(i - 1));
}


template<class T>
constexpr T cmax(T x, T y)
{
	return x > y ? x : y;
}

typedef Vector2f vec2;
typedef Vector3f vec3;
typedef Vector4f vec4;


// handy value clamping to 0 - 1 range
float saturate(float value)
{
	return clamp(value, 0.0f, 1.0f);
}


// phong (lambertian) diffuse term
float phong_diffuse()
{
	return (1.0f / PI);
}


// compute fresnel specular factor for given base specular and product
// product could be NdV or VdH depending on used technique
vec3 fresnel_factor(vec3 f0, float product)
{
	return mix(f0, vec3(1.0f), pow(1.01f - product, 5.0f));
}


// following functions are copies of UE4
// for computing cook-torrance specular lighting terms

float D_blinn(float roughness, float NdH)
{
	float m	 = roughness * roughness;
	float m2 = m * m;
	float n	 = 2.0f / m2 - 2.0f;
	return (n + 2.0f) / (2.0f * PI) * pow(NdH, n);
}

float D_beckmann(float roughness, float NdH)
{
	float m	   = roughness * roughness;
	float m2   = m * m;
	float NdH2 = NdH * NdH;
	return exp((NdH2 - 1.0f) / (m2 * NdH2)) / (PI * m2 * NdH2 * NdH2);
}

float D_GGX2(float roughness, float NdH)
{
	float m	 = roughness * roughness;
	float m2 = m * m;
	float d	 = (NdH * m2 - NdH) * NdH + 1.0f;
	return m2 / (PI * d * d);
}

float G_schlick(float roughness, float NdV, float NdL)
{
	float k = roughness * roughness * 0.5f;
	float V = NdV * (1.0f - k) + k;
	float L = NdL * (1.0f - k) + k;
	return 0.25f / (V * L);
}


// simple phong specular calculation with normalization
vec3 phong_specular(vec3 V, vec3 L, vec3 N, vec3 specular, float roughness)
{
	vec3  R	   = reflect(-L, N);
	float spec = max(0.0f, dot(V, R));

	float k = 1.999f / (roughness * roughness);

	return min(1.0f, 3.0f * 0.0398f * k) * pow(spec, min(10000.0f, k)) * specular;
}

// simple blinn specular calculation with normalization
vec3 blinn_specular(float NdH, vec3 specular, float roughness)
{
	float k = 1.999f / (roughness * roughness);

	return min(1.0f, 3.0f * 0.0398f * k) * pow(NdH, min(10000.0f, k)) * specular;
}

// cook-torrance specular calculation
vec3 cooktorrance_specular(float NdL, float NdV, float NdH, vec3 specular, float roughness)
{
#ifdef COOK_BLINN
	float D = D_blinn(roughness, NdH);
#endif

#ifdef COOK_BECKMANN
	float D = D_beckmann(roughness, NdH);
#endif

#ifdef COOK_GGX
	float D = D_GGX2(roughness, NdH);
#endif

	float G = G_schlick(roughness, NdV, NdL);

	float rim = mix(1.0f - roughness /** material.w*/ * 0.9f, 1.0f, NdV);

	return (1.0f / rim) * specular * G * D;
}

// https://gist.github.com/galek/53557375251e1a942dfa
// A: light attenuation
Color4f PBRShading(float metallic, float roughness, Vector3f N, Vector3f V, Vector3f L, Vector3f H, float A, Vector3f Albedo, RxImage* BRDFTexture, const RxImageCube* EnvTexture)
{
	//// point light direction to point in view space
	//vec3 local_light_pos = (view_matrix * (/*world_matrix */ light_pos)).xyz;

	//// light attenuation
	//float A = 20.0 / dot(local_light_pos - v_pos, local_light_pos - v_pos);

	//// L, V, H vectors
	//vec3 L	= normalize(local_light_pos - v_pos);
	//vec3 V	= normalize(-v_pos);
	//vec3 H	= normalize(L + V);
	vec3 nn = normalize(N);

	/*vec3   nb  = normalize(v_binormal);
	mat3x3 tbn = mat3x3(nb, cross(nn, nb), nn);*/


	//vec2 texcoord = v_texcoord;


	// normal map
#if USE_NORMAL_MAP
	// tbn basis
	vec3 N = tbn * (texture2D(norm, texcoord).xyz * 2.0 - 1.0);
#else
	N			= nn;
#endif

	// albedo/specular base
#if USE_ALBEDO_MAP
	vec3 base = texture2D(tex, texcoord).xyz;
#else
	vec3  base		= Albedo;
#endif

//	// roughness
//#if USE_ROUGHNESS_MAP
//	float roughness = texture2D(spec, texcoord).y * material.y;
//#else
//	float roughness = material.y;
//#endif

	//// material params
	//float metallic = material.x;

	// mix between metal and non-metal material, for non-metal
	// constant base specular factor of 0.04 grey is used
	vec3 specular = mix(vec3(0.04), base, metallic);

	//// diffuse IBL term
	////    I know that my IBL cubemap has diffuse pre-integrated value in 10th MIP level
	////    actually level selection should be tweakable or from separate diffuse cubemap
	//mat3x3 tnrm	   = transpose(normal_matrix);
	//vec3   envdiff = textureCubeLod(envd, tnrm * N, 10).xyz;
	vec3 envdiff = vec3(textureCube(EnvTexture, N, 10));

	//// specular IBL term
	////    11 magic number is total MIP levels in cubemap, this is simplest way for picking
	////    MIP level from roughness value (but it's not correct, however it looks fine)
	vec3 refl	 = /*tnrm **/ reflect(-V, N);
	/*vec3 envspec = textureCubeLod(
				   EnvTexture, refl, max(roughness * 11.0f, textureQueryLod(envd, refl).y))
				   .xyz;*/

	vec3 envspec = vec3(textureCubeLod(EnvTexture, refl, 0));

	////vec3 envspec = vec3(0.3f);

	// compute material reflectance

	float NdL = max(0.0f, dot(N, L));
	float NdV = max(0.001f, dot(N, V));
	float NdH = max(0.001f, dot(N, H));
	float HdV = max(0.001f, dot(H, V));
	float LdV = max(0.001f, dot(L, V));

	// fresnel term is common for any, except phong
	// so it will be calcuated inside ifdefs


#ifdef PHONG
	// specular reflectance with PHONG
	vec3 specfresnel = fresnel_factor(specular, NdV);
	vec3 specref	 = phong_specular(V, L, N, specfresnel, roughness);
#endif

#ifdef BLINN
	// specular reflectance with BLINN
	vec3 specfresnel = fresnel_factor(specular, HdV);
	vec3 specref	 = blinn_specular(NdH, specfresnel, roughness);
#endif

#ifdef COOK_GGX
	// specular reflectance with COOK-TORRANCE
	vec3 specfresnel = fresnel_factor(specular, HdV);
	vec3 specref	 = cooktorrance_specular(NdL, NdV, NdH, specfresnel, roughness);
#endif

	specref *= vec3(NdL);

	// diffuse is common for any model
	vec3 diffref = (vec3(1.0) - specfresnel) * phong_diffuse() * NdL;


	// compute lighting
	vec3 reflected_light = vec3(0);
	vec3 diffuse_light	 = vec3(0); // initial value == constant ambient light

	// point light
	vec3 light_color = vec3(1.0) * A;
	reflected_light += specref * light_color;
	diffuse_light += diffref * light_color;

	// IBL lighting
	vec2 brdf	 = vec2(texture2D(BRDFTexture, vec2(roughness, 1.0 - NdV)));
	vec3 iblspec = min(vec3(0.99), fresnel_factor(specular, NdV) * brdf.x + brdf.y);
	reflected_light += iblspec * envspec;
	diffuse_light += envdiff * (1.0f / PI);

	// final result
	vec3 result =
	diffuse_light * mix(base, vec3(0.0), metallic) +
	reflected_light;

	return vec4(result, 1);
}