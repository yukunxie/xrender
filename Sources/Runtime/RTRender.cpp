
#include "RTRender.h"

#include <embree4/rtcore.h>
#include <limits>
#include <iostream>
#include <thread>
#include <simd/varying.h>
#include<omp.h> 

#include <math/vec2.h>
#include <math/vec2fa.h>

#include <math/vec3.h>
#include <math/vec3fa.h>

#include <bvh/bvh.h>
#include <geometry/trianglev.h>

#include <iostream>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "Camera.h"
#include "Tools.h"
#include "Light.h"
#include "MeshProxy.h"
#include "Graphics/PhysicalImage.h"
#include "Graphics/RxSampler.h"
#include "Object/MeshComponent.h"
#include <glm/glm.hpp>
#include <glm/geometric.hpp>  

extern std::map<int, MeshComponent*> GMeshComponentProxies;

glm::vec3 triangleBitangent(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3)
{
	glm::vec3 edge1	   = v2 - v1;
	glm::vec3 edge2	   = v3 - v1;
	glm::vec2 deltaUV1 = uv2 - uv1;
	glm::vec2 deltaUV2 = uv3 - uv1;

	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	glm::vec3 bitangent;
	bitangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	bitangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	bitangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	return bitangent;
}

glm::vec3 triangleTangent(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3)
{
	glm::vec3 edge1	   = v2 - v1;
	glm::vec3 edge2	   = v3 - v1;
	glm::vec2 deltaUV1 = uv2 - uv1;
	glm::vec2 deltaUV2 = uv3 - uv1;

	float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

	glm::vec3 tangent;
	tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
	tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
	tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

	return tangent;
}

Vector3f interpolateNormal(float u, float v, Vector3f p0, Vector3f p1, Vector3f p2, Vector3f n0, Vector3f n1, Vector3f n2)
{
	glm::vec3 p = (1 - u - v) * p0 + u * p1 + v * p2;
	glm::vec3 tangent0, bitangent0, tangent1, bitangent1, tangent2, bitangent2;
	//glm::triangleTangent(p0, p1, p2, tangent0, bitangent0);
	//glm::triangleTangent(p1, p2, p0, tangent1, bitangent1);
	//glm::triangleTangent(p2, p0, p1, tangent2, bitangent2);

	return {};
}

static Color4f _ProcessRayHitResult(PhysicalImage* renderTarget, PBRRender& pbrRender, const GlobalConstantBuffer& cGlobalBuffer, const BatchBuffer& cBatchBuffer, const ShadingBuffer& cShadingBuffer, vec3 viewDir, vec2 uv, int geomID, int primID)
{
	Assert(geomID != RTC_INVALID_GEOMETRY_ID);

	if (geomID != 0)
	{
		auto  meshProxy = GMeshComponentProxies[geomID];

		float u = uv.x;
		float v = uv.y;

		uint32 v0, v1, v2;
		std::tie(v0, v1, v2) = meshProxy->GetGeometry()->GetIndexBuffer()->GetVerticesByPrimitiveId(primID);

		Vector2f uv0 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector2f>(VertexBufferAttriKind::TEXCOORD, v0);
		Vector2f uv1 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector2f>(VertexBufferAttriKind::TEXCOORD, v1);
		Vector2f uv2 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector2f>(VertexBufferAttriKind::TEXCOORD, v2);

		Vector3f n0 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::NORMAL, v0);
		Vector3f n1 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::NORMAL, v1);
		Vector3f n2 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::NORMAL, v2);

		Vector3f p0 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::POSITION, v0);
		Vector3f p1 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::POSITION, v1);
		Vector3f p2 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::POSITION, v2);

		Vector3f t0 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::TANGENT, v0);
		Vector3f t1 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::TANGENT, v1);
		Vector3f t2 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::TANGENT, v2);

		Vector3f b0 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::BITANGENT, v0);
		Vector3f b1 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::BITANGENT, v1);
		Vector3f b2 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::BITANGENT, v2);


#if 0
		Vector2f uv		= uv0 * u + uv1 * v + uv2 * (1.0f - u - v);
		Vector3f normal = glm::normalize(n0) * u + glm::normalize(n1) * v + glm::normalize(n2) * (1.0f - u - v);
		Vector3f pos	= p0 + u * (p1 - p0) + v * (p2 - p0);
#else
		Vector2f uv		= (1.0f - u - v) * uv0  + uv1 * u + uv2 *v;
		Vector3f normal = (1.0f - u - v) * glm::normalize(n0)  + glm::normalize(n1) * u + glm::normalize(n2)*v;
		Vector3f pos	= (1.0f - u - v) * p0 + u * p1  + v * p2;
#endif

		normal = glm::normalize(normal);

		Vector3f T = (1.0f - u - v)* t0  + t1 * u + t2 * v;
		Vector3f B = (1.0f - u - v)* b0  + b1 * u + b2 * v;

		glm::mat3 tbnMatrix = glm::mat3(T, B, normal);
		glm::mat3 invTBNMatrix = glm::inverse(tbnMatrix);
		glm::mat3 normalMatrix = glm::transpose(invTBNMatrix);

		auto material = meshProxy->GetMaterial();

		Color4f frag = pbrRender.Render(cGlobalBuffer, cBatchBuffer, cShadingBuffer, pos, normal, uv, normalMatrix, material);

		return frag;
	}
	else
	{
		auto meshProxy = GMeshComponentProxies[geomID];

		float u = uv.x;
		float v = uv.y;

		uint32 v0, v1, v2;
		std::tie(v0, v1, v2) = meshProxy->GetGeometry()->GetIndexBuffer()->GetVerticesByPrimitiveId(primID);

		Vector3f p0 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::POSITION, v0);
		Vector3f p1 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::POSITION, v1);
		Vector3f p2 = meshProxy->GetGeometry()->GetAttributeByIndex<Vector3f>(VertexBufferAttriKind::POSITION, v2);

		std::swap(u, v);
		////Vector3f pos = (1.0f - u - v) * p0 + u * p1 + v * p2;
		Vector3f pos = u * p0 + v * p1 + (1 - u - v) * p2;


		return pbrRender.RenderSkybox(cGlobalBuffer, (pos));
	}
}

void RTRender(Vector3f pos, Vector3f foucs, Vector3f up, float fov, PhysicalImage* renderTarget, RTCScene scene, PBRRender& pbrRender)
{
	SCOPED_PROFILING_GUARD("RayTracingRender");
	int		 width	= renderTarget->GetWidth();
	int		 height = renderTarget->GetHeight();

	//Vector3f pos   = { 10.0f, 0.0f, 0.0f };
	//Vector3f foucs = { 0.0f, 0.0f, 0.0f };
	//Vector3f up	   = { 0, 1, 0 };

	embree::Camera camera;
	camera.from		  = embree::Vec3fa(pos.x, pos.y, pos.z);
	camera.to		  = embree::Vec3fa(foucs.x, foucs.y, foucs.z);
	camera.up		  = embree::Vec3fa(up.x, up.y, up.z);
	camera.fov		  = fov;
	camera.handedness = embree::Camera::Handedness::RIGHT_HANDED;

	embree::ISPCCamera ispcCamera = camera.getISPCCamera(width, height);

	GlobalConstantBuffer cGlobalBuffer;
	{
		cGlobalBuffer.EyePos		= Vector4f(pos.x, pos.y, pos.z, 1.0f);
		cGlobalBuffer.SunLight		= Vector4f(-10.0f, -10.0f, -10.0f, 0.0f);
		cGlobalBuffer.SunLightColor = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
		cGlobalBuffer.ViewMatrix	= glm::lookAtRH(pos, foucs, up);
		cGlobalBuffer.ProjMatrix;
	}

	glm::vec3 center(0.0f, 0.0f, 0.0f); // 立方体中心位置
	float	  length = 20.0f;			// 立方体边长的一半

	// 定义8个顶点坐标
	glm::vec3 vertex0(center.x - length, center.y - length, center.z - length);
	glm::vec3 vertex1(center.x + length, center.y - length, center.z - length);
	glm::vec3 vertex2(center.x - length, center.y + length, center.z - length);
	glm::vec3 vertex3(center.x + length, center.y + length, center.z - length);
	glm::vec3 vertex4(center.x - length, center.y - length, center.z + length);
	glm::vec3 vertex5(center.x + length, center.y - length, center.z + length);
	glm::vec3 vertex6(center.x - length, center.y + length, center.z + length);
	glm::vec3 vertex7(center.x + length, center.y + length, center.z + length);

	// 将8个顶点坐标放入一个std::vector列表中
	std::vector<glm::vec3> vertices = { vertex0, vertex1, vertex2, vertex3, vertex4, vertex5, vertex6, vertex7 };

	for (auto pos : vertices)
	{
		cGlobalBuffer.Lights.emplace_back(pos, vec3(300.0f, 300.0f, 300.0f));
	}

	BatchBuffer cBatchBuffer;
	{
		cBatchBuffer.WorldMatrix = TMat4x4(1.0f);
	}

#define HITBOCKSIZE 8
	alignas(64) int validMasks[HITBOCKSIZE];
	memset(validMasks, 0xFF, sizeof(validMasks));

#if HITBOCKSIZE == 1
#	define RTC_ARRAY_PREFIX(ARGS) (&(ARGS))
#else
#	define RTC_ARRAY_PREFIX(ARGS) ARGS
#endif

	constexpr int NUM_CORE = 8;
#pragma omp parallel for num_threads(NUM_CORE)
	for (int h = 0; h < height; ++h)
	{

		for (int b = 0; b < width / HITBOCKSIZE; b++)
		{
			ShadingBuffer cShadingBuffer;
			{
				cShadingBuffer.baseColorFactor = Vector4f(1.0f);
				cShadingBuffer.emissiveFactor  = Vector4f(0.0f);
				cShadingBuffer.metallicFactor  = 0.0f;
				cShadingBuffer.roughnessFactor = 0.5f;
			}

#if HITBOCKSIZE == 8
			RTCRayHit8 rayhits;
#elif HITBOCKSIZE == 4
			RTCRayHit4 rayhits;
#elif HITBOCKSIZE == 1
			RTCRayHit rayhits;
#else
			Assert(false);
#endif
			
			for (int i = 0; i < HITBOCKSIZE; ++i)
			{
				int	 w	 = b * HITBOCKSIZE + i;
				auto dir = embree::Vec3fa(embree::normalize(w * ispcCamera.xfm.l.vx + h * ispcCamera.xfm.l.vy + ispcCamera.xfm.l.vz));

				RTC_ARRAY_PREFIX(rayhits.ray.org_x)[i]  = ispcCamera.xfm.p.x;
				RTC_ARRAY_PREFIX(rayhits.ray.org_y)[i]  = ispcCamera.xfm.p.y;
				RTC_ARRAY_PREFIX(rayhits.ray.org_z)[i]  = ispcCamera.xfm.p.z;
				RTC_ARRAY_PREFIX(rayhits.ray.dir_x)[i]  = dir.x;
				RTC_ARRAY_PREFIX(rayhits.ray.dir_y)[i]  = dir.y;
				RTC_ARRAY_PREFIX(rayhits.ray.dir_z)[i]  = dir.z;
				RTC_ARRAY_PREFIX(rayhits.ray.tnear)[i]  = 0.f;
				RTC_ARRAY_PREFIX(rayhits.ray.tfar)[i]	  = 2000;
				RTC_ARRAY_PREFIX(rayhits.ray.time)[i]	   = HITBOCKSIZE;
				RTC_ARRAY_PREFIX(rayhits.ray.mask)[i]	  = 0xFFFFFFFF;
				RTC_ARRAY_PREFIX(rayhits.hit.geomID)[i]  = RTC_INVALID_GEOMETRY_ID;
			}

#if HITBOCKSIZE == 8
			rtcIntersect8(validMasks, scene, &rayhits);
#elif HITBOCKSIZE == 4
			rtcIntersect4(validMasks, scene, &rayhits);
#elif HITBOCKSIZE == 1
			rtcIntersect1(scene, &rayhits);
#else
			Assert(false);
#endif

			for (int i = 0; i < HITBOCKSIZE; ++i)
			{
				int		w	   = b * HITBOCKSIZE + i;
				float	x	   = RTC_ARRAY_PREFIX(rayhits.ray.dir_x)[i];
				float	y	   = RTC_ARRAY_PREFIX(rayhits.ray.dir_y)[i];
				float	z	   = RTC_ARRAY_PREFIX(rayhits.ray.dir_z)[i];
				float	u	   = RTC_ARRAY_PREFIX(rayhits.hit.u)[i];
				float	v	   = RTC_ARRAY_PREFIX(rayhits.hit.v)[i];
				int		primID = RTC_ARRAY_PREFIX(rayhits.hit.primID)[i];
				int		geomID = RTC_ARRAY_PREFIX(rayhits.hit.geomID)[i];
				Color4f frag   = _ProcessRayHitResult(renderTarget, pbrRender, cGlobalBuffer, cBatchBuffer, cShadingBuffer, vec3(x, y, z), vec2(u, v), geomID, primID);
				renderTarget->WritePixel(w, h, frag);
			}
		}
	}
}