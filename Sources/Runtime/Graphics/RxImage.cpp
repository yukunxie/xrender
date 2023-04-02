#include "RxImage.h"

#include <span>

#define STB_IMAGE_WRITE_IMPLEMENTATION 1
#define STB_IMAGE_IMPLEMENTATION	   1

#include <stb_image.h>
#include <stb_image_write.h>

#include "Utils/FileSystem.h"
#include "Loader/ImageLoader.h"

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