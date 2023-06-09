#pragma once

#include <embree4/rtcore.h>
#include <limits>
#include <iostream>
#include <span>
#include <simd/varying.h>

#include <math/vec2.h>
#include <math/vec2fa.h>

#include <math/vec3.h>
#include <math/vec3fa.h>

#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtx/compatibility.hpp"

#include "Utils/TimeProfiler.h"

#include <memory>

typedef std::uint8_t  uint8;
typedef std::uint16_t uint16;
typedef std::uint32_t uint32;
typedef std::uint64_t uint64;

typedef std::vector<std::uint8_t> TData;

template<typename T0, typename T1>
T1 madd(T0 x, T1 y, T1 z)
{
	return (x * y) + z;
}

#ifdef _DEBUG
#	ifndef Assert
#		define Assert(cond, ...) assert(cond)
#	endif
//#define Assert(cond, format, ...)  do {if (!(cond)) AssertImpl(__FILE__, "", __LINE__, format, ##__VA_ARGS__); } while(0)
#else
#	define Assert(cond, ...) ((void)0)
#endif

#ifndef M_PI
#define M_PI 3.141592653589793f
#endif

enum EMeshRayHitFlags : uint32
{
	EMeshRayHitFlags_Render		   = 1 << 0,
	EMeshRayHitFlags_CastShadow	   = 1 << 1,
	EMeshRayHitFlags_RecieveShadow = 1 << 2,
};

typedef glm::vec2 Vector2f;
typedef glm::vec3 Vector3f;
typedef glm::vec4 Vector4f;

typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::vec4 vec4;

typedef glm::vec2 Color2f;
typedef glm::vec3 Color3f;
typedef glm::vec4 Color4f;

typedef glm::u8vec2 Color2B;
typedef glm::u8vec3 Color3B;
typedef glm::u8vec4 Color4B;

typedef glm::u32vec2 Vector2I;
typedef glm::u32vec3 Vector3I;
typedef glm::u32vec4 Vector4I;

typedef glm::u32vec2 Extent2D;
typedef glm::u32vec3 Extent3D;

typedef Vector2f TVector2;
typedef Vector3f TVector3;
typedef Vector4f TVector4;

typedef glm::mat4x4 TMat4x4;
typedef glm::mat3x4 TMat3x4;
typedef glm::mat3x3 TMat3x3;

struct CameraInfo
{
	TVector3 Position;
	TVector3 Foucs;
	TVector3 Up;
	float	 Fov;
};

struct Triangle
{
	uint32 v0, v1, v2;
};

struct TriangleUint16
{
	uint16 v0, v1, v2;
};

enum class IndexType
{
	UINT32,
	UINT16
};

enum class TextureFormat
{
	INVALID,

	// 8-bit formats
	R8UNORM,
	R8SNORM,
	R8UINT,
	R8SINT,

	// 16-bit formats
	R16UINT,
	R16SINT,
	R16FLOAT,
	RG8UNORM,
	RG8SNORM,
	RG8UINT,
	RG8SINT,

	// 32-bit formats
	R32UINT,
	R32SINT,
	R32FLOAT,
	RG16UINT,
	RG16SINT,
	RG16FLOAT,
	RGB8UNORM,
	RGB8UNORM_SRGB,
	RGBA8UNORM,
	RGBA8UNORM_SRGB,
	RGBA8SNORM,
	RGBA8UINT,
	RGBA8SINT,
	BGRA8UNORM,
	BGRA8UNORM_SRGB,

	// Packed 32-bit formats
	RGB10A2UNORM,
	RG11B10FLOAT,

	// 64-bit formats
	RG32UINT,
	RG32SINT,
	RG32FLOAT,
	RGBA16UINT,
	RGBA16SINT,
	RGBA16FLOAT,

	// 128-bit formats
	RGBA32UINT,
	RGBA32SINT,
	RGBA32FLOAT,
	RGB32FLOAT,

	// Depth and stencil formats
	DEPTH32FLOAT,
	DEPTH24PLUS,
	DEPTH24PLUS_STENCIL8
};

enum class MaterialParameterType
{
	FLOAT,
	FLOAT2,
	FLOAT3,
	FLOAT4,

	INT,
	INT2,
	INT3,
	INT4,

	BOOL,
	BOOL1,
	BOOL2,
	BOOL3,

	MAT4,
	MAT3,
	MAT2,
	MAT4x3,
	MAT4x2,
	MAT3x4,
	MAT2x4,
	MAT3x2,
	MAT2x3,

	BUFFER,
	SAMPLER2D,
	TEXTURE2D,
};

enum class InputAttributeFormat
{
	FLOAT,
	FLOAT2,
	FLOAT3,
	FLOAT4,
};


enum VertexBufferAttriKind
{
	INVALID	  = 0,
	POSITION  = 1 << 0,  // xyz
	DIFFUSE	  = 1 << 1,  // rgba 4byte
	TEXCOORD  = 1 << 2,  // uv 2floats
	NORMAL	  = 1 << 3,  // xyz
	TANGENT	  = 1 << 4, // xyz
	BINORMAL  = 1 << 5, // xyz
	BITANGENT = 1 << 6, // xyz
	TEXCOORD1 = 1 << 7, // uv2 2floats
	TEXCOORD2 = 1 << 8, // uv2 2floats
};

constexpr std::uint32_t VertexBufferKindCount = 7;

enum class MaterailBindingObjectType
{
	BUFFER,
	SAMPLER2D,
	TEXTURE2D,
};

enum InputAttributeLocation
{
	IA_LOCATION_POSITION  = 0,
	IA_LOCATION_NORMAL	  = 1,
	IA_LOCATION_TEXCOORD  = 2,
	IA_LOCATION_DIFFUSE	  = 3,
	IA_LOCATION_TANGENT	  = 4,
	IA_LOCATION_BINORMAL  = 5,
	IA_LOCATION_BITANGENT = 6,
	IA_LOCATION_TEXCOORD2 = 7,
};

enum class ETechniqueType
{
	TShading = 0,
	TGBufferGen,
	TShadowMapGen,
	TOutline,
	TSkyBox,
	TVolumeCloud,

	TMaxCount,
};

enum ERenderSet : std::uint64_t
{
	ERenderSet_Opaque	   = 1 << 0,
	ERenderSet_Transparent = 1 << 1,
	ERenderSet_SkyBox	   = 1 << 2,
	ERenderSet_PostProcess = 1 << 3,
};

enum ETechniqueMask : std::uint64_t
{
	TShading	= 1 << (uint32)ETechniqueType::TShading,
	TGBufferGen = 1 << (uint32)ETechniqueType::TGBufferGen,
};

using TechniqueFlags = std::uint64_t;

constexpr uint32 kMaxAttachmentCount = 6;

MaterialParameterType ToMaterialParameterType(const char* text);

std::uint32_t GetInputAttributeLocation(VertexBufferAttriKind kind);

std::uint32_t GetFormatSize(InputAttributeFormat format);

enum LogLevel
{
	Verbose = 0,
	Info,
	Debug,
	Warning,
	Error,
};

void _OutputLog(LogLevel level, const char* format, ...);

#define LOGI(format, ...) _OutputLog(LogLevel::Info, format, ##__VA_ARGS__);

#define LOGW(format, ...) _OutputLog(LogLevel::Warning, format, ##__VA_ARGS__);

#define LOGE(format, ...) _OutputLog(LogLevel::Error, format, ##__VA_ARGS__);

/*! aligned allocation */
void* alignedMalloc(size_t size, size_t align);
void  alignedFree(void* ptr);

#define ALIGNED_STRUCT(align)             \
	void* operator new(size_t size)        \
	{                                      \
		return alignedMalloc(size, align); \
	}                                      \
	void operator delete(void* ptr)        \
	{                                      \
		alignedFree(ptr);                  \
	}                                      \
	void* operator new[](size_t size)      \
	{                                      \
		return alignedMalloc(size, align); \
	}                                      \
	void operator delete[](void* ptr)      \
	{                                      \
		alignedFree(ptr);                  \
	}

#define ALIGNED_STRUCT_USM(align)            \
	void* operator new(size_t size)           \
	{                                         \
		return alignedUSMMalloc(size, align); \
	}                                         \
	void operator delete(void* ptr)           \
	{                                         \
		alignedUSMFree(ptr);                  \
	}                                         \
	void* operator new[](size_t size)         \
	{                                         \
		return alignedUSMMalloc(size, align); \
	}                                         \
	void operator delete[](void* ptr)         \
	{                                         \
		alignedUSMFree(ptr);                  \
	}

#define ALIGNED_CLASS(align) \
public:                       \
	ALIGNED_STRUCT(align)    \
private:

#define ALIGNED_CLASS_USM(align) \
public:                           \
	ALIGNED_STRUCT_USM(align)    \
private: