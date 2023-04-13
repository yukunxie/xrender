#pragma once

#include "Graphics/PhysicalImage.h"

int Renderer(const PhysicalImage* renderTarget, const std::function<void(float, float)>& mouseMoveHandler);