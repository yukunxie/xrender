#pragma once

#include "Types.h"
#include "Graphics/RxImage.h"

Color4f PBRShading(float metallic, float roughness, Vector3f N, Vector3f V, Vector3f L, Vector3f H, float A, Vector3f Albedo, RxImage* BRDFTexture, const RxImageCube* EnvTexture);