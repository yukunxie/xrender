
#pragma once

#include "Types.h"

class RxImage;

typedef std::shared_ptr<RxImage> TexturePtr;

class RxImage
{
public:
	RxImage(){};

	RxImage(const char* filename);

	RxImage(uint32 width, uint32 height, uint32 channels, const uint8* data = nullptr);

	~RxImage()
	{
		if (mData)
		{
			free(mData);
		}
	}

	Color4B SamplePixel(float u, float v) const;
		 
	Color4B SamplePixel(uint32 x, uint32 y) const;
		 
	Color4B SamplePixel(Vector2I xy) const;

	void WritePixel(int x, int y, std::uint8_t r, std::uint8_t g, std::uint8_t b);

	void WritePixel(int x, int y, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);

	void SaveToFile(const char* filename);

	int GetWidth() const
	{
		return mWidth;
	}

	int GetHeight() const
	{
		return mHeight;
	}

	int GetChannelNum() const
	{
		return mChannels;
	}

	const std::uint8_t* GetData() const
	{
		return mData;
	}

	static std::shared_ptr<RxImage> LoadTextureFromUri(const std::string& filename);

	static std::shared_ptr<RxImage> LoadCubeTexture(const std::string& cubeTextureName);

	static std::shared_ptr<RxImage> LoadTextureFromData(const std::uint8_t* data, std::uint32_t byteLength, const std::string& debugName = "");

	static std::shared_ptr<RxImage> LoadTextureFromData(uint32 width, uint32 height, uint32 component, const uint8* data, uint32 byteLength, const std::string& debugName = "");

	static std::shared_ptr<RxImage> Create3DNoiseTexture();

protected:
	uint32		  mWidth;
	uint32		  mHeight;
	uint32		  mChannels;
	std::uint8_t* mData = nullptr;
};