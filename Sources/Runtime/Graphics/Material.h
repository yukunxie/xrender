
#pragma once

#include "Types.h"
#include "Texture.h"
#include <map>

struct MaterialParameter
{
	std::string			   Name;
	MaterialParameterType  Format;
	std::unique_ptr<uint8> Data;
};

struct LightData
{
	Vector3f Position;
	Vector3f Color;
};

struct GlobalConstantBuffer
{
	Vector4f			   EyePos;
	Vector4f			   SunLight;
	Vector4f			   SunLightColor;
	TMat4x4				   ViewMatrix;
	TMat4x4				   ProjMatrix;
	std::vector<LightData> Lights;
};

struct EnvironmentTextures
{
	TexturePtr EnvTexture;
	TexturePtr BRDFTexture;
	TexturePtr SphericalEnvTexture;
	TexturePtr IrradianceTexture;
};

struct ShadingBuffer
{
	Vector4f emissiveFactor;
	Vector4f baseColorFactor;
	float	 metallicFactor;
	float	 roughnessFactor;
};

struct BatchBuffer
{
	TMat4x4 WorldMatrix;
};


class Material
{
public:
	Material(const std::string& configFilename = "");

	bool SetFloat(const std::string& name, std::uint32_t offset, std::uint32_t count, const float* data);

	bool SetTexture(const std::string& name, TexturePtr texture);

	TexturePtr GetTexture(const std::string& name) const
	{
		auto it = mTextures.find(name);
		return it != mTextures.end() ? it->second : nullptr;
	}

public:
	std::map<std::string, MaterialParameter> mParameters;
	std::map<std::string, TexturePtr> mTextures;
};