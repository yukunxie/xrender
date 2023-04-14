
#pragma once

#include "Types.h"
#include "Graphics/RxSampler.h"
#include <algorithm>

#ifndef PI
#define PI		3.1415926535897932384626433832795f
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
	vec3 AOMask = vec3(1.0f);
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
FORCEINLINE float D_GGX(float dotNH, float roughness)
{
	float alpha	 = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom	 = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2) / (PI * denom * denom);
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
FORCEINLINE vec3 F_Schlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}
FORCEINLINE vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0f - cosTheta, 5.0f);
}