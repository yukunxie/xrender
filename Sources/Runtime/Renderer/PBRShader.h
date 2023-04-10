#pragma once

#include "Types.h"
#include "Graphics/RxImage.h"
#include "Graphics/Material.h"
#include "ShadingBase.h"


Color4f PBRShading(float metallic, float roughness, const TMat3x3& normalMatrix, Vector3f N, Vector3f V, Vector3f L, Vector3f H, float A, Vector3f Albedo, PhysicalImage* BRDFTexture, const RxImageCube* EnvTexture);


Color4f PBRShading(const GlobalConstantBuffer& cGlobalBuffer, const GBufferData& gBufferData, PhysicalImage* BRDFTexture, const RxImageCube* EnvTexture);