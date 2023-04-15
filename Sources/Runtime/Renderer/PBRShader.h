#pragma once

#include "Types.h"
#include "Graphics/PhysicalImage.h"
#include "Graphics/Material.h"
#include "ShadingBase.h"

Color4f PBRShading(const GlobalConstantBuffer& cGlobalBuffer, const GBufferData& gBufferData, const EnvironmentTextures& gEnvironmentData);