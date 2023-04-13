
#pragma once

#include "Types.h"

class PhysicalImage;

class PhysicalImage
{
protected:
	PhysicalImage()
	{
	}

	virtual PhysicalImage* Clone(uint32 width, uint32 height, uint32 channels) const
	{
		return new PhysicalImage(width, height, channels, nullptr);
	}

public:
	PhysicalImage(uint32 width, uint32 height, uint32 channels, uint32 channelSize)
		: mWidth(width)
		, mHeight(height)
		, mChannels(channels)
	{
		size_t size = channelSize * width * height * channels;
		mData		= (uint8*)malloc(size);
		memset(mData, 0, size);
	};

	PhysicalImage(const char* filename);

	PhysicalImage(uint32 width, uint32 height, uint32 channels, const uint8* data = nullptr);

	virtual ~PhysicalImage()
	{
		if (mData)
		{
			free(mData);
		}
	}

	virtual Color4f ReadPixel(int x, int y) const;

	virtual void WritePixel(int w, int h, Color4f color);

	virtual void SaveToFile(const std::string& filename);

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

	/*static std::shared_ptr<PhysicalImage> LoadTextureFromUri(const std::string& filename);

	static std::shared_ptr<PhysicalImage> LoadCubeTexture(const std::string& cubeTextureName);

	static std::shared_ptr<PhysicalImage> LoadTextureFromData(const std::uint8_t* data, std::uint32_t byteLength, const std::string& debugName = "");

	static std::shared_ptr<PhysicalImage> LoadTextureFromData(uint32 width, uint32 height, uint32 component, const uint8* data, uint32 byteLength, const std::string& debugName = "");

	static std::shared_ptr<PhysicalImage> Create3DNoiseTexture();*/

	virtual PhysicalImage* DownSample() const;

protected:
	uint32		  mWidth;
	uint32		  mHeight;
	uint32		  mChannels;
	std::uint8_t* mData = nullptr;
};

class PhysicalImage32F : public PhysicalImage
{
public:
	PhysicalImage32F(const char* hdrFilename);

	PhysicalImage32F(uint32 width, uint32 height, uint32 channels, const float* data = nullptr);

	virtual PhysicalImage* Clone(uint32 width, uint32 height, uint32 channels) const override
	{
		return new PhysicalImage32F(width, height, channels, nullptr);
	}

	virtual Color4f ReadPixel(int w, int h) const;

	virtual void WritePixel(int w, int h, Color4f color) override;

	virtual void SaveToFile(const std::string& filename) override;
};

class RxImageCube : public PhysicalImage
{
public:
	/*RxImageCube()
	{
	}*/
	RxImageCube(const char* filename);

	virtual PhysicalImage* Clone(uint32 width, uint32 height, uint32 channels) const override
	{
		Assert(false);
		return nullptr;
	}

	Color4f ReadPixel(Vector3f dir) const;


protected:
	std::shared_ptr<PhysicalImage> mTextures[6];
};