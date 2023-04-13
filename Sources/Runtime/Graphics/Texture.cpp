#include "Texture.h"
#include "Utils/FileSystem.h"

Texture2D::Texture2D(const std::string& filename)
{
	PhysicalImage* image = nullptr;
	if (filename.ends_with(".hdr"))
	{
		image = new PhysicalImage32F(filename.c_str());
	}
	else
	{
		image = new PhysicalImage(filename.c_str());
	}
	mImages.emplace_back(image);
}

Texture2D::Texture2D(PhysicalImage* image)
{
	mImages.emplace_back(image);
	image = nullptr;
}

Texture2D::Texture2D(uint32 width, uint32 height, TextureFormat textureFormat, const uint8* data)
	: mTextureFormat(textureFormat)
{
	PhysicalImage* image = nullptr;
	switch (textureFormat)
	{
	case TextureFormat::RGBA8UNORM:
		image = new PhysicalImage(width, height, 4, data);
		break;
	case TextureFormat::RGB8UNORM:
		image = new PhysicalImage(width, height, 3, data);
		break;
	case TextureFormat::RGBA32FLOAT:
		image = new PhysicalImage32F(width, height, 4, (const float*)data);
		break;
	case TextureFormat::RGB32FLOAT:
		image = new PhysicalImage32F(width, height, 3, (const float*)data);
		break;
	default:
		Assert(false);
	}
	mImages.emplace_back(image);
}

void Texture2D::AutoGenerateMipmaps()
{
	Assert(!mImages.empty());
	if (mImages.size() > 1)
		return;
	auto* image = mImages.back().get();
	while (image->GetWidth() > 1 || image->GetHeight() > 1)
	{
		auto nxtImage = image->DownSample();
		mImages.emplace_back(nxtImage);
		image = nxtImage;
	}
}

void Texture2D::SaveToFile(const std::string& filenameWithoutSuffix) const
{
	std::string suffix = ".png";
	/*if (mTextureFormat == TextureFormat::RGBA32FLOAT)
	{
		suffix = ".hdr";
	}
	else if (mTextureFormat == TextureFormat::RGB32FLOAT)
	{
		suffix = ".hdr";
	}*/

	if (mImages.size() == 1)
	{
		mImages[0]->SaveToFile((filenameWithoutSuffix + suffix).c_str());
	}
	else
	{
		for (size_t i = 0; i < mImages.size(); ++i)
		{
			std::string filename = filenameWithoutSuffix + "-mip-" + std::to_string(i) + suffix;
			mImages[i]->SaveToFile(filename.c_str());
			break;
		}
	}
}

TextureCube::TextureCube(const std::string& cubefilename)
{
	static const std::vector<std::string> sImageExts = { ".jpg", ".png", ".tag", ".bmp", };
	constexpr uint32 kFaceNum			  = 6;
	const char*							  kFaceNames[kFaceNum] = { "nx", "px", "py", "ny", "nz", "pz" };
	std::string		 fileExt			  = "";
	for (const auto& ext : sImageExts)
	{
		std::string filename = "Textures\\CubeTextures\\"s + cubefilename + "\\"s + "nx"s + ext;
		if (!FileSystem::GetInstance()->GetAbsFilePath(filename.c_str()).empty())
		{
			fileExt = ext;
			break;
		}
	}

	Assert(!fileExt.empty());

	for (int face = 0; face < kFaceNum; ++face)
	{
		std::string filename = "Textures\\CubeTextures\\"s + cubefilename + "\\"s + kFaceNames[face] + fileExt;
		mFaces[face]		 = std::make_shared<Texture2D>(filename.c_str());
	}

	AutoGenerateMipmaps();
}

TextureCube::TextureCube(uint32 width, uint32 height, TextureFormat textureFormat, const uint8* data)
{
	for (int i = 0; i < 6; ++i)
	{
		mFaces[i] = std::make_shared<Texture2D>(width, height, textureFormat, nullptr);
	}
}

void TextureCube::WritePixel(uint32 face, uint32 w, uint32 h, Color4f color)
{
	mFaces[face]->GetImageWithMip(0)->WritePixel(w, h, color);
}

void TextureCube::AutoGenerateMipmaps()
{
	for (auto t : mFaces)
	{
		t->AutoGenerateMipmaps();
	}
}

void TextureCube::SaveToFile(const std::string& filenameWithoutSuffix) const
{
	static const char* CubeFaceNames[6] = { "px", "nx", "py", "ny", "pz", "nz" };

	for (int i = 0; i < 6; ++i)
	{
		mFaces[i]->SaveToFile(filenameWithoutSuffix + "-" + CubeFaceNames[i]);
	}
}
