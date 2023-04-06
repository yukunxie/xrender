#include "RxImage.h"

#include <span>
#include <string>
using namespace std::string_literals;

#define STB_IMAGE_WRITE_IMPLEMENTATION 1
#define STB_IMAGE_IMPLEMENTATION	   1

#include <stb_image.h>
#include <stb_image_write.h>

#include "Utils/FileSystem.h"
#include "Loader/ImageLoader.h"
#include "RxSampler.h"

static const std::vector<std::string> sImageExts = {
	".jpg",
	".png",
	".tag",
	".bmp",
};


RxImage::RxImage(const char* filename)
{
	std::string absFilename = FileSystem::GetInstance()->GetAbsFilePath(filename);
	int			w, h, c;
	mData	  = stbi_load(absFilename.c_str(), &w, &h, &c, 4);
	mWidth	  = w;
	mHeight	  = h;
	mChannels = c;
}

RxImage::RxImage(uint32 width, uint32 height, uint32 channels, const uint8* data)
	: mWidth(width)
	, mHeight(height)
	, mChannels(channels)
{
	uint32 size = width * height * channels;
	mData		= (std::uint8_t*)malloc(size);
	if (data)
	{
		memcpy(mData, data, size);
	}
}

Color4B RxImage::SamplePixel(Vector2I xy) const
{
	return SamplePixel((uint32)xy.x, (uint32)xy.y);
}

Color4B RxImage::SamplePixel(uint32 x, uint32 y) const
{
	if (mChannels == 3)
	{
		std::span<Color3B> pixels{ (Color3B*)mData, (size_t)(mWidth * mHeight) };
		return { pixels[y * mWidth + x], 255 };
	}
	else
	{
		std::span<Color4B> pixels{ (Color4B*)mData, (size_t)(mWidth * mHeight) };
		return pixels[y * mWidth + x];
	}
}

Color4B RxImage::SamplePixel(float u, float v) const
{
	uint32 x = std::min((uint32)mWidth - 1, uint32(u * mWidth));
	uint32 y = std::min((uint32)mHeight - 1, uint32(v * mHeight));
	return SamplePixel(x, y);
}

void RxImage::WritePixel(int x, int y, std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
	if (mChannels == 3)
	{
		std::span<Color3B> pixels { (Color3B*)mData, (size_t)(mWidth * mHeight) };
		pixels[y * mWidth + x] = {r, g, b};
	}
	else
	{
		std::span<Color4B> pixels{ (Color4B*)mData, (size_t)(mWidth * mHeight) };
		pixels[y * mWidth + x] = { r, g, b, 255 };
	}
}

void RxImage::WritePixel(int x, int y, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
{
	if (mChannels == 3)
	{
		std::span<Color3B> pixels{ (Color3B*)mData, (size_t)(mWidth * mHeight) };
		pixels[y * mWidth + x] = { r, g, b };
	}
	else
	{
		std::span<Color4B> pixels{ (Color4B*)mData, (size_t)(mWidth * mHeight) };
		pixels[y * mWidth + x] = { r, g, b, a };
	}
}

void RxImage::SaveToFile(const char* filename)
{
	stbi_write_png(filename, mWidth, mHeight, mChannels, mData, mWidth * mChannels);
}

// static
std::shared_ptr<RxImage> RxImage::LoadTextureFromUri(const std::string& filename)
{
	return (ImageLoader::LoadTextureFromUri(filename));
}

std::shared_ptr<RxImage> RxImage::LoadCubeTexture(const std::string& cubeTextureName)
{
	return (ImageLoader::LoadCubeTexture(cubeTextureName));
}

std::shared_ptr<RxImage> RxImage::LoadTextureFromData(const std::uint8_t* data, std::uint32_t byteLength, const std::string& debugName)
{
	return (ImageLoader::LoadTextureFromData(data, byteLength, debugName));
}

std::shared_ptr<RxImage> RxImage::LoadTextureFromData(uint32 width, uint32 height, uint32 component, const uint8* data, uint32 byteLength, const std::string& debugName)
{
	return (ImageLoader::LoadTextureFromData(width, height, component, data, byteLength, debugName));
}

std::shared_ptr<RxImage> RxImage::Create3DNoiseTexture()
{
	return (ImageLoader::Create3DFloatTexture(256, 256, 128));
}

RxImageCube::RxImageCube(const char* cubeTextureName)
{
	constexpr uint32 kFaceNum			  = 6;
	const char*		 kFaceNames[kFaceNum] = { "px", "nx", "py", "ny", "pz", "nz" };
	std::string		 fileExt			  = "";
	for (const auto& ext : sImageExts)
	{
		std::string filename = "Textures\\CubeTextures\\"s + cubeTextureName + "\\"s + "nx"s + ext;
		if (!FileSystem::GetInstance()->GetAbsFilePath(filename.c_str()).empty())
		{
			fileExt = ext;
			break;
		}
	}

	Assert(!fileExt.empty());

	for (int face = 0; face < kFaceNum; ++face)
	{
		std::string	 filename = "Textures\\CubeTextures\\"s + cubeTextureName + "\\"s + kFaceNames[face] + fileExt;
		mTextures[face]		 = std::make_shared<RxImage>(filename.c_str());
	}
}

Color4f RxImageCube::SamplePixel(Vector3f TexCoords) const
{
	//dir = glm::normalize(dir);

	/*const glm::vec3 FRONT  = glm::vec3(0.0f, 0.0f, 1.0f);
	const glm::vec3 BACK   = glm::vec3(0.0f, 0.0f, -1.0f);
	const glm::vec3 LEFT   = glm::vec3(-1.0f, 0.0f, 0.0f);
	const glm::vec3 RIGHT  = glm::vec3(1.0f, 0.0f, 0.0f);
	const glm::vec3 TOP	   = glm::vec3(0.0f, 1.0f, 0.0f);
	const glm::vec3 BOTTOM = glm::vec3(0.0f, -1.0f, 0.0f);

	float dotFront = glm::dot(dir, FRONT);
	float angle	   = glm::acos(dotFront);

	const int NUM_TEX  = 6;
	int		  texIndex = static_cast<int>(angle / (glm::two_pi<float>() / NUM_TEX)) % NUM_TEX;*/

	auto posx = mTextures[0];
	auto negx = mTextures[1];

	auto posy = mTextures[2];
	auto negy = mTextures[3];

	auto posz = mTextures[4];
	auto negz = mTextures[5];

	float mag = glm::max(glm::max(abs(TexCoords.x), abs(TexCoords.y)), abs(TexCoords.z));
	if (mag == abs(TexCoords.x))
	{
		if (TexCoords.x > 0)
			return texture2D(posx.get(), Vector2f((TexCoords.z + 1) / 2, (TexCoords.y + 1) / 2));
		else if (TexCoords.x < 0)
			return texture2D(negx.get(), Vector2f((TexCoords.z + 1) / 2, (TexCoords.y / mag + 1) / 2));
	}
	else if (mag == abs(TexCoords.y))
	{
		if (TexCoords.y > 0)
			return texture2D(posy.get(), Vector2f((TexCoords.x + 1) / 2, (TexCoords.z + 1) / 2));
		else if (TexCoords.y < 0)
			return texture2D(negy.get(), Vector2f((TexCoords.x + 1) / 2, (TexCoords.z + 1) / 2));
	}
	else if (mag == abs(TexCoords.z))
	{
		if (TexCoords.z > 0)
			return texture2D(posz.get(), Vector2f((TexCoords.x + 1) / 2, (TexCoords.y + 1) / 2));
		else if (TexCoords.z < 0)
			return texture2D(negz.get(), Vector2f((TexCoords.x + 1) / 2, (TexCoords.y + 1) / 2));
	}
}