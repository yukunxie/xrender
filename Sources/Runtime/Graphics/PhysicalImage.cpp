#include "PhysicalImage.h"

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


PhysicalImage::PhysicalImage(const char* filename)
{
	std::string absFilename = FileSystem::GetInstance()->GetAbsFilePath(filename);
	int			w, h, c;
	mData	  = stbi_load(absFilename.c_str(), &w, &h, &c, 4);
	mWidth	  = w;
	mHeight	  = h;
	mChannels = c;
}

PhysicalImage::PhysicalImage(uint32 width, uint32 height, uint32 channels, const uint8* data)
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

Color4f PhysicalImage::ReadPixel(int w, int h) const
{
	w = glm::clamp(w, 0, (int)mWidth - 1);
	h = glm::clamp(h, 0, (int)mHeight - 1);

	if (mChannels == 3)
	{
		std::span<Color3B> pixels{ (Color3B*)mData, (size_t)(mWidth * mHeight) };
		Color3B			   c = pixels[h * mWidth + w];
		return Color4f(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f);
	}
	else
	{
		std::span<Color4B> pixels{ (Color4B*)mData, (size_t)(mWidth * mHeight) };
		Color4B			   c = pixels[h * mWidth + w];
		return Color4f(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
	}
}

void PhysicalImage::WritePixel(int w, int h, Color4f color)
{
	color = glm::clamp(color, 0.0f, 1.0f);

	uint8 r = int(color.r * 255);
	uint8 g = int(color.g * 255);
	uint8 b = int(color.b * 255);
	uint8 a = int(color.a * 255);

	if (mChannels == 3)
	{
		std::span<Color3B> pixels{ (Color3B*)mData, (size_t)(mWidth * mHeight) };
		pixels[h * mWidth + w] = { r, g, b };
	}
	else
	{
		std::span<Color4B> pixels{ (Color4B*)mData, (size_t)(mWidth * mHeight) };
		pixels[h * mWidth + w] = { r, g, b, a };
	}
}

void PhysicalImage::SaveToFile(const std::string& filename)
{
	stbi_write_png(filename.c_str(), mWidth, mHeight, mChannels, mData, mWidth * mChannels);
}

PhysicalImage* PhysicalImage::DownSample() const
{
	if (GetWidth() == 1 && GetHeight() == 1)
	{
		return new PhysicalImage(GetWidth(), GetHeight(), GetChannelNum(), mData);
	}

	uint32 twidth  = std::max(1, GetWidth() / 2);
	uint32 theight = std::max(1, GetWidth() / 2);

	PhysicalImage* img = Clone(twidth, theight, GetChannelNum());

	for (uint32 w = 0; w < twidth; ++w)
	{
		for (uint32 h = 0; h < theight; ++h)
		{
			Color4f sum(0);
			float	count = 0;
			for (int w1 = -1; w1 <=1; ++w1)
			{
				for (int h1 = -1; h1 <=1; ++h1)
				{
					sum += ReadPixel(w * 2 + w1, h * 2 + h1);
					count++;
				}
			}
			sum = sum / count;
			img->WritePixel((int)w, (int)h, sum);
		}
	}
	return img;
}

//// static
//std::shared_ptr<PhysicalImage> PhysicalImage::LoadTextureFromUri(const std::string& filename)
//{
//	return (ImageLoader::LoadTextureFromUri(filename));
//}
//
//std::shared_ptr<PhysicalImage> PhysicalImage::LoadCubeTexture(const std::string& cubeTextureName)
//{
//	return (ImageLoader::LoadCubeTexture(cubeTextureName));
//}
//
//std::shared_ptr<PhysicalImage> PhysicalImage::LoadTextureFromData(const std::uint8_t* data, std::uint32_t byteLength, const std::string& debugName)
//{
//	return (ImageLoader::LoadTextureFromData(data, byteLength, debugName));
//}

//std::shared_ptr<PhysicalImage> PhysicalImage::LoadTextureFromData(uint32 width, uint32 height, uint32 component, const uint8* data, uint32 byteLength, const std::string& debugName)
//{
//	return (ImageLoader::LoadTextureFromData(width, height, component, data, byteLength, debugName));
//}
//
//std::shared_ptr<PhysicalImage> PhysicalImage::Create3DNoiseTexture()
//{
//	return (ImageLoader::Create3DFloatTexture(256, 256, 128));
//}


PhysicalImage32F::PhysicalImage32F(const char* hdrFilename)
{
	std::string absFilename = FileSystem::GetInstance()->GetAbsFilePath(hdrFilename);
	int			w, h, c;
	mData	  = (std::uint8_t*)stbi_loadf(absFilename.c_str(), &w, &h, &c, 0);
	mWidth	  = w;
	mHeight	  = h;
	mChannels = c;
}

PhysicalImage32F::PhysicalImage32F(uint32 width, uint32 height, uint32 channels, const float* data)
	: PhysicalImage(width, height, channels, (uint32)sizeof(float))
{
	if (data)
	{
		size_t size = width * height * channels * sizeof(float);
		memcpy(mData, data, size);
	}
}

Color4f PhysicalImage32F::ReadPixel(int w, int h) const
{
	w = glm::clamp(w, 0, (int)mWidth - 1);
	h = glm::clamp(h, 0, (int)mHeight - 1);
	if (mChannels == 3)
	{
		std::span<Color3f> pixels{ (Color3f*)mData, (size_t)(mWidth * mHeight) };
		return { pixels[h * mWidth + w], 1.0f };
	}
	else
	{
		std::span<Color4f> pixels{ (Color4f*)mData, (size_t)(mWidth * mHeight) };
		return pixels[h * mWidth + w];
	}
}

void PhysicalImage32F::WritePixel(int w, int h, Color4f color)
{
	if (mChannels == 3)
	{
		std::span<Color3f> pixels{ (Color3f*)mData, (size_t)(mWidth * mHeight) };
		pixels[h * mWidth + w] = color.rgb;
	}
	else
	{
		std::span<Color4f> pixels{ (Color4f*)mData, (size_t)(mWidth * mHeight) };
		pixels[h * mWidth + w] = color;
	}
}

void PhysicalImage32F::SaveToFile(const std::string& filename)
{
	if (filename.ends_with(".hdr"))
	{
		stbi_write_hdr(filename.c_str(), mWidth, mHeight, mChannels, (const float*)mData); 
	}
	else
	{
		PhysicalImage image(mWidth, mHeight, 4);
		for (int w = 0; w < mWidth; ++w)
		{
			for (int h = 0; h < mHeight; ++h)
			{
				Color3f color = ReadPixel(w, h).xyz;
				//	// HDR tonemapping
				color = color / (color + vec3(1.0f));
				color = pow(color, vec3(1.0 / 2.2));
				image.WritePixel(w, h, Color4f(color, 1.0f));
			}
		}
		image.SaveToFile(filename);
	}
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
		std::string filename = "Textures\\CubeTextures\\"s + cubeTextureName + "\\"s + kFaceNames[face] + fileExt;
		mTextures[face]		 = std::make_shared<PhysicalImage>(filename.c_str());
	}
}

Color4f RxImageCube::ReadPixel(Vector3f TexCoords) const
{
	// dir = glm::normalize(dir);

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

	/*auto posx = mTextures[0];
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
	}*/
	return {};
}