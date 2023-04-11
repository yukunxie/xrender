
#pragma once

#include "Types.h"
#include "RxImage.h"

class Texture
{
public:
	Texture()  = default;
	~Texture() = default;

	virtual void AutoGenerateMipmaps() = 0;

	virtual void SaveToFile(const std::string& filenameWithoutSuffix) const = 0;
};

class Texture2D : public Texture
{
public:
	Texture2D(PhysicalImage* image);

	Texture2D(const std::string& filename);

	Texture2D(uint32 width, uint32 height, TextureFormat textureFormat, const uint8* data = nullptr);

	virtual void AutoGenerateMipmaps() override;

	std::shared_ptr<PhysicalImage> GetImageWithMip(uint32 mip) const 
	{
		Assert(mip < mImages.size());
		return mImages[mip];
	}

	virtual void SaveToFile(const std::string& filenameWithoutSuffix) const override;

protected:
	TextureFormat								mTextureFormat;
	std::vector<std::shared_ptr<PhysicalImage>> mImages;
};

class TextureCube : public Texture
{
public:
	TextureCube(uint32 width, uint32 height, TextureFormat textureFormat, const uint8* data = nullptr);

	void WritePixel(uint32 face, uint32 w, uint32 h, Color4f color);

	virtual void AutoGenerateMipmaps() override;

	virtual void SaveToFile(const std::string& filenameWithoutSuffix) const override;

protected:
	std::shared_ptr<Texture2D> mFaces[6];
};
