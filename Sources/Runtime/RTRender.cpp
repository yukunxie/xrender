
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
#include "Graphics/RxImage.h"
#include "Graphics/RxSampler.h"
#include "Object/MeshComponent.h"

extern std::map<int, MeshComponent*> GMeshComponentProxies;

static Color4f _ProcessRayHitResult(RxImage* renderTarget, PBRRender& pbrRender, const GlobalConstantBuffer& cGlobalBuffer, const BatchBuffer& cBatchBuffer, const ShadingBuffer& cShadingBuffer, float u, float v, int geomID, int primID)
{
	if (geomID != RTC_INVALID_GEOMETRY_ID)
	{
		auto  meshProxy = GMeshComponentProxies[geomID];

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

		Vector2f uv		= uv0 * u + uv1 * v + uv2 * (1.0f - u - v);
		Vector3f normal = n0 * u + n1 * v + n2 * (1.0f - u - v);
		Vector3f pos	= p0 + u * (p1 - p0) + v * (p2 - p0);

		auto material = meshProxy->GetMaterial();

		Color4f frag = pbrRender.Render(cGlobalBuffer, cBatchBuffer, cShadingBuffer, pos, normal, uv, material);

		return frag;
	}
	else
	{
		return Color4f(0.0f);
	}
}

void RTRender(Vector3f pos, Vector3f foucs, Vector3f up, RxImage* renderTarget, RTCScene scene, PBRRender& pbrRender)
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
	camera.fov		  = 45;
	camera.handedness = embree::Camera::Handedness::LEFT_HANDED;

	embree::ISPCCamera ispcCamera = camera.getISPCCamera(width, height);

	GlobalConstantBuffer cGlobalBuffer;
	{
		cGlobalBuffer.EyePos		= Vector4f(pos.x, pos.y, pos.z, 1.0f);
		cGlobalBuffer.SunLight		= Vector4f(-10.0f, -10.0f, -10.0f, 0.0f);
		cGlobalBuffer.SunLightColor = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
		cGlobalBuffer.ViewMatrix;
		cGlobalBuffer.ProjMatrix;
	}

	BatchBuffer cBatchBuffer;
	{
		cBatchBuffer.WorldMatrix = TMat4x4(1.0f);
	}

	constexpr int NUM_CORE = 8;
#pragma omp parallel for num_threads(NUM_CORE)
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; x++)
		{
			auto	  dir = embree::Vec3fa(embree::normalize(x * ispcCamera.xfm.l.vx + y * ispcCamera.xfm.l.vy + ispcCamera.xfm.l.vz));
			RTCRayHit rayhit;

			rayhit.ray.org_x  = ispcCamera.xfm.p.x;
			rayhit.ray.org_y  = ispcCamera.xfm.p.y;
			rayhit.ray.org_z  = ispcCamera.xfm.p.z;
			rayhit.ray.dir_x  = dir.x;
			rayhit.ray.dir_y  = dir.y;
			rayhit.ray.dir_z  = dir.z;
			rayhit.ray.tnear  = 0.f;
			rayhit.ray.tfar	  = 2000;
			rayhit.ray.time	  = 1;
			rayhit.ray.mask	  = 0xFFFFFFFF;
			rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

			rtcIntersect1(scene, &rayhit);

			ShadingBuffer cShadingBuffer;
			{
				cShadingBuffer.baseColorFactor = Vector4f(1.0f);
				cShadingBuffer.emissiveFactor  = Vector4f(0.0f);
				cShadingBuffer.metallicFactor  = 0.0f;
				cShadingBuffer.roughnessFactor = 0.5f;
			}

			float u		 = rayhit.hit.u;
			float v		 = rayhit.hit.v;
			int	  primID = rayhit.hit.primID;
			int	  geomID = rayhit.hit.geomID;
			Color4f frag = _ProcessRayHitResult(renderTarget, pbrRender, cGlobalBuffer, cBatchBuffer, cShadingBuffer, u, v, geomID, primID);
			Color4B color(int(frag.x * 255), int(frag.y * 255), int(frag.z * 255), 255);
			renderTarget->WritePixel(x, y, color.r, color.g, color.b);
		}
	}
}