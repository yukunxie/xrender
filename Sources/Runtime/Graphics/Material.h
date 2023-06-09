
#pragma once

#include "Types.h"
#include "Texture.h"
#include "Renderer/ShadingBase.h"
#include <map>


class MeshComponent;
class Geometry;
class Material;
class Raytracer;

struct EnvironmentTextures
{
	TexturePtr EnvTexture;
	TexturePtr BRDFTexture;
	TexturePtr SphericalEnvTexture;
	TexturePtr IrradianceTexture;
};

EnvironmentTextures* GetEnvironmentData();

struct MaterialParameter
{
	std::string			   Name;
	MaterialParameterType  Format;
	std::vector<uint8> Data;
};

struct LightData
{
	Vector3f Position;
	Vector3f Color;
	Vector3f ShapeColor;
};

struct GlobalConstantBuffer
{
	Vector4f			   EyePos;
	Vector4f			   SunLight;
	Vector4f			   SunLightColor;
	TMat4x4				   ViewMatrix;
	TMat4x4				   ProjMatrix;
	TMat4x4				   ViewProjMatrix;
	std::vector<LightData> Lights;
};

struct ShadingBuffer
{
	Vector4f emissiveFactor;
	Vector4f baseColorFactor;
	float	 metallicFactor;
	float	 roughnessFactor;
};

struct VertexOutputData
{
	bool HasTexcoord  = false;
	bool HasNormal	  = false;
	bool HasTangent	  = false;
	bool HasBiTangent = false;

	vec3 Position;
	vec2 Texcoord0;
	vec3 Normal;
	vec3 Tangent;
	vec3 BiTangent;
};

struct RenderOutputData
{
	vec4  Color;
	float Depth;
	vec3  WorldPosition;
	vec3  Normal;
};

struct BatchBuffer
{
	TMat4x4 WorldMatrix;
};

class RenderCore
{
public:
	virtual ~RenderCore()
	{
	}

public:
	template<typename AttrType>
	FORCEINLINE AttrType InterpolateAttribute(vec2 barycenter, const AttrType& p0, const AttrType& p1, const AttrType& p2)
	{
		return barycenter.x * p1 + barycenter.y * p2 + (1.0f - barycenter.x - barycenter.y) * p2;
	}

	static VertexOutputData InterpolateAttributes(vec2 barycenter, vec3 ng, const TMat4x4& worldMatrix, const Geometry* mesh, int primId) noexcept;

public:
	virtual RenderOutputData Execute(const Raytracer*			 rayTracer,
							const GlobalConstantBuffer& cGlobalBuffer,
							const VertexOutputData&		vertexData,
							class Material*				material) noexcept = 0;
};

class RenderCorePBR : public RenderCore
{
public:
	RenderOutputData Execute(const Raytracer*			 rayTracer,
					const GlobalConstantBuffer& cGlobalBuffer,
					const VertexOutputData&		vertexData,
					class Material*				material) noexcept override;

protected:
	RenderOutputData Shading(const Raytracer*			 rayTracer,
					const GlobalConstantBuffer& cGlobalBuffer,
					const GBufferData&			gBufferData,
					const EnvironmentTextures&	gEnvironmentData) const noexcept;
};

class RenderCoreSkybox : public RenderCore
{
public:
	RenderOutputData Execute(const Raytracer*			 rayTracer,
					const GlobalConstantBuffer& cGlobalBuffer,
					const VertexOutputData&		vertexData,
					class Material*				material) noexcept override;
};

class RenderCoreUnlit : public RenderCore
{
public:
	RenderOutputData Execute(const Raytracer*			 rayTracer,
					const GlobalConstantBuffer& cGlobalBuffer,
					const VertexOutputData&		vertexData,
					class Material*				material) noexcept override;
};

typedef std::shared_ptr<RenderCore> RenderCorePtr;

enum EAlphaMode
{
	EAlphaMode_Opaque,
	EAlphaMode_Blend,
	EAlphaMode_Mask,
};


class Material
{
public:
	Material();
	Material(const std::string& configFilename = "");

	bool SetFloat(const std::string& name, std::uint32_t offset, std::uint32_t count, const float* data);

	bool SetTexture(const std::string& name, TexturePtr texture);

	TexturePtr GetTexture(const std::string& name) const
	{
		auto it = mTextures.find(name);
		return it != mTextures.end() ? it->second : nullptr;
	}

	void SetRenderCore(const RenderCorePtr& renderCore)
	{
		mRenderCore = renderCore;
	}

	const RenderCorePtr& GetRenderCore() const
	{
		return mRenderCore;
	}

	ShadingBuffer GetShadingBuffer() const;

	bool HasParameter(const std::string& name) const noexcept
	{
		return mParameters.contains(name);
	}

	float GetFloat(const std::string& name) const;

	vec2 GetVec2(const std::string& name) const;

	vec3 GetVec3(const std::string& name) const;

	vec4 GetVec4(const std::string& name) const;

	void SetAlphaMode(EAlphaMode alphaMode)
	{
		mAlphaMode = alphaMode;
	}

	EAlphaMode GetAlphaMode() const 
	{
		return mAlphaMode;
	}

	void SetAlphaCutoff(float cutoff)
	{
		mAlphaCutoff = cutoff;
	}

	float GetAlphaCutoff() const
	{
		return mAlphaCutoff;
	}

	void SetDoubleSide(bool doubleSide)
	{
		mDoubleSide = doubleSide;
	}

	bool GetDoubleSide() const
	{
		return mDoubleSide;
	}

public:
	std::map<std::string, MaterialParameter> mParameters;
	std::map<std::string, TexturePtr>		 mTextures;

protected:
	RenderCorePtr mRenderCore;
	EAlphaMode	  mAlphaMode   = EAlphaMode_Opaque;
	float		  mAlphaCutoff = 0.5f;
	bool		  mDoubleSide  = true;
};