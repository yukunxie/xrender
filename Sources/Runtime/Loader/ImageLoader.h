#pragma once

#include "Types.h"
#include "Graphics/Texture.h"

namespace ImageLoader
{
	TexturePtr LoadTextureFromUri(const std::string& filename);
	TexturePtr LoadCubeTexture(const std::string& cubeTextureName);
	TexturePtr LoadTextureFromData(const std::uint8_t* data, std::uint32_t byteLength, const std::string& debugName = "");
	TexturePtr LoadTextureFromData(uint32 width, uint32 height, uint32 component, const uint8* data, uint32 byteLength, const std::string& debugName = "");
	TexturePtr Create3DFloatTexture(uint32 width, uint32 height, uint32 depth);
}
