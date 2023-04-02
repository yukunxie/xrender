#pragma once

#include "Graphics/RxImage.h"
#include "Renderer/PBRRender.h"

void RTRender(Vector3f pos, Vector3f foucs, Vector3f up, RxImage* renderTarget, RTCScene scene, PBRRender& pbrRender);