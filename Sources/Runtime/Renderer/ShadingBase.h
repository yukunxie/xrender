
#pragma once

#include "Types.h"
#include "Graphics/RxSampler.h"
#include <algorithm>

#ifndef PI
#define PI		3.1415926535897932384626433832795f
#endif

#ifndef INV_PI
#	define INV_PI (1/PI)
#endif

#ifndef Epsilon
#define Epsilon 0.00001f
#endif

struct GBufferData
{
	vec3 Albedo;
	vec3 WorldNormal;
	vec3 Position;
	vec3 Material; // r: metallic, g: roughness, b: Shading Model
	vec3 EmissiveColor = vec3(0);
	vec3 AOMask		   = vec3(1.0f);
};

template<class T>
FORCEINLINE constexpr T pow2(T x)
{
	return x * x;
}

template<typename T1, typename T2>
FORCEINLINE T1 mix(T1 a, T1 b, T2 c)
{
	return glm::mix(a, b, c);
}

template<typename T1, typename T2>
FORCEINLINE T1 clamp(T1 a, T2 b, T2 c)
{
	return glm::clamp(a, b, c);
}

template<typename T1>
FORCEINLINE T1 max(T1 a, T1 b)
{
	return glm::max(a, b);
}

template<typename T1>
FORCEINLINE T1 min(T1 a, T1 b)
{
	return glm::min(a, b);
}

template<class T>
FORCEINLINE constexpr size_t nextPowerOfTwo(T i)
{
	// Equivalent to 2^ceil(log2(i))
	return i == 0 ? 0 : 1ull << (sizeof(T) * 8 - std::countl_zero(i - 1));
}

template<class T>
FORCEINLINE constexpr T cmax(T x, T y)
{
	return x > y ? x : y;
}

// handy value clamping to 0 - 1 range
FORCEINLINE float saturate(float value)
{
	return clamp(value, 0.0f, 1.0f);
}

// Normal Distribution function --------------------------------------
// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
FORCEINLINE float D_GGX(float NdotH, float roughness)
{
	// We are doing our approximations based on the Trowbridge-Reits GGX:
	float a2	 = roughness * roughness;
	NdotH		 = max(NdotH, 0.0f);
	float NdotH2 = NdotH * NdotH;

	float nom	= a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom		= PI * denom * denom;

	// The normal distribution function returns a value indicating how much o the surface's
	// microfacets are aligned to the incoming halfway vector.
	return nom / denom;
}

// ----------------------------------------------------------------------------
FORCEINLINE float D_GGX(vec3 N, vec3 H, float roughness)
{
	return D_GGX(glm::clamp(dot(N, H), 0.0f, 0.99f), roughness);
}

FORCEINLINE float D_GGX2(float roughness, float NdH)
{
	return D_GGX(NdH, roughness);
}

// From http://filmicgames.com/archives/75
FORCEINLINE vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

FORCEINLINE vec3 ACESToneMapping(vec3 color, float adapted_lum)
{
	const float A = 2.51f;
	const float B = 0.03f;
	const float C = 2.43f;
	const float D = 0.59f;
	const float E = 0.14f;

	color *= adapted_lum;
	return (color * (A * color + B)) / (color * (C * color + D) + E);
}

// Geometric Shadowing function --------------------------------------
FORCEINLINE float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r	 = (roughness + 1.0);
	float k	 = (r * r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

FORCEINLINE float G_schlick(float roughness, float NdV, float NdL){
	return G_SchlicksmithGGX(NdL, NdV, roughness);
}

// Fresnel function ----------------------------------------------------
FORCEINLINE vec3 F_Schlick(float VoH, vec3 SpecularColor)
{
	float Fc = glm::pow(1.0f - VoH, 5.0f); // 1 sub, 3 mul
	// return Fc + (1 - Fc) * SpecularColor;		// 1 add, 3 mad

	// Anything less than 2% is physically impossible and is instead considered to be shadowing
	return saturate(50.0 * SpecularColor.g) * Fc + (1 - Fc) * SpecularColor;

	//return glm::max(vec3(0), F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f));
}
FORCEINLINE vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0f - cosTheta, 5.0f);
}
// ----------------------------------------------------------------------------
FORCEINLINE float GeometrySchlickGGX(float NdotV, float k)
{
	float nom	= NdotV;
	float denom = NdotV * (1.0 - k) + k;
	return nom / denom;
}

FORCEINLINE float GeometrySmith(float NdotV, float NdotL, float Roughness)
{
	float squareRoughness = Roughness * Roughness;
	float k				  = squareRoughness / 2.0f;
	//pow(squareRoughness + 1, 2) / 8;
	float ggx1			  = GeometrySchlickGGX(NdotV, k); // 视线方向的几何遮挡
	float ggx2			  = GeometrySchlickGGX(NdotL, k); // 光线方向的几何阴影
	return ggx1 * ggx2;
}

// The Fresnel Equation
FORCEINLINE  vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	// cosTheta, the dot product between the surface's normal and the halfway vector.
	// F0, vector describing the base reflectivity of the surface

	/*
	 We are doing our approximation based on the Fesnel-Schlick:
	 The Fresnel Equtation describes the ration of light that gets reflected,
	 over the light that gets reflacted.
	*/

	return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.f);
}

// --------------------------------------------------------`--------------------
FORCEINLINE float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV			  = max(dot(N, V), 0.0f);
	float NdotL			  = max(dot(N, L), 0.0f);
	return GeometrySmith(NdotV, NdotL, roughness);
}

FORCEINLINE vec2 SampleSphericalMap(vec3 v)
{
	static const vec2 invAtan = vec2(0.1591f, 0.3183f);
	vec2			  uv	  = vec2(glm::atan(v.z, v.x), asin(v.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}