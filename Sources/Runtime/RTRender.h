#pragma once

#include "Graphics/PhysicalImage.h"
#include "Renderer/PBRRender.h"

void RTRender(Vector3f pos, Vector3f foucs, Vector3f up, float fov, PhysicalImage* renderTarget, RTCScene scene, PBRRender& pbrRender);