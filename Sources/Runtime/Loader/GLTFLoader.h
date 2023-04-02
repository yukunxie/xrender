#pragma once

#include "Types.h"
#include "Object/Entity.h"
#include "Graphics/Material.h"

namespace GLTFLoader
{
	std::vector<Entity*> LoadModelFromGLTF(const std::string& filename);
}
