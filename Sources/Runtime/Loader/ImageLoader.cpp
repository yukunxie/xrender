#include "ImageLoader.h"
#include "stb_image.h"
//#include "ktx/include/ktx.h"
#include "Utils/FileSystem.h"

#include <random>
#include <numeric>

uint32 GetMipMapLevels(uint32 n)
{
    uint32 level = 0;
    for (; n != 0; level++, n = n >> 1);
    return level;
}

std::vector<std::vector<uint8>> GenerateMipMap(uint32 width, uint32 height, const uint8* pixels, uint32 channelNum)
{
    Assert(width == height && width > 1);
    // Pow of two
    Assert((width & (width - 1)) == 0);

    std::vector<std::vector<uint8>> mipmaps;

    uint32 pixelSize = channelNum;
    std::vector<uint8> mipmap0(width * height * pixelSize);
    memcpy(mipmap0.data(), pixels, width * height * pixelSize);
    mipmaps.push_back(std::move(mipmap0));

    for (width = width >> 1, height = height >> 1; width > 0 && height > 0; width = width >> 1, height = height >> 1)
    {
        uint32 lwh = width << 1;
        uint32 lBytesPerRow = lwh * pixelSize;
        uint32 bytesPerRow = width * pixelSize;

        std::vector<uint8> mipmap(width * height * pixelSize);
        //auto mipmap = std::make_shared<uint8>(new uint8[width * height * pixelSize]);
        const uint8* pData = mipmaps.back().data();

        for (uint32 h = 0; h < height; h++)
        {
            for (uint32 w = 0; w < width; w++)
            {
                uint32 rgba[4] = { 0, 0, 0, 0 };
                const uint8* ph0 = pData + (2 * h * lBytesPerRow + 2 * w * pixelSize);
                const uint8* ph1 = pData + ((2 * h + 1) * lBytesPerRow + 2 * w * pixelSize);
                for (uint32 c = 0; c < pixelSize; ++c)
                {
                    rgba[c] += ph0[c];
                    rgba[c] += ph0[c + pixelSize];
                    rgba[c] += ph1[c + pixelSize];
                    rgba[c] += ph1[c + pixelSize];

                    // average
                    rgba[c] /= 4;

                    uint8* buffer = mipmap.data();
                    buffer[h * bytesPerRow + w * pixelSize + c] = rgba[c];
                }
            }
        }
        mipmaps.push_back(std::move(mipmap));
    }
    return std::move(mipmaps);
}

namespace ImageLoader
{
    static const std::vector<std::string> sImageExts = { ".jpg", ".png", ".tag", ".bmp", };

    struct ImageData
    {
        std::string debugName;
		TextureFormat format;
        uint32 baseWidth;
        uint32 baseHeight;
        uint32 numLevels;
        uint32 numFaces;
        uint32 byteLength;
        const uint8* bytes;
        // layer * 1000 * 1000 + face * 1000 + mipmaps
        std::unordered_map<uint32, uint32> offsets;

        uint32 GetOffset(uint32 layer, uint32 face, uint32 level) const
        {
            uint32 key = layer * 1000 * 1000 + face * 1000 + level;
            return offsets.find(key)->second;
        }

        void SetOffset(uint32 layer, uint32 face, uint32 level, uint32 offset)
        {
            uint32 key = layer * 1000 * 1000 + face * 1000 + level;
            offsets[key] = offset;
        }
    };

    TexturePtr CreateCubeTexture(const ImageData& imageData)
    {
		Assert(false);
		return {};
        //Assert(imageData.format == gfx::TextureFormat::RGBA8UNORM);

        //gfx::TextureDescriptor descriptor;
        //{
        //    descriptor.sampleCount = 1;
        //    descriptor.format = gfx::TextureFormat::RGBA8UNORM;
        //    descriptor.usage = gfx::TextureUsage::SAMPLED | gfx::TextureUsage::COPY_DST;
        //    descriptor.size = { imageData.baseWidth, imageData.baseHeight, 1 };
        //    descriptor.arrayLayerCount = imageData.numFaces;
        //    descriptor.mipLevelCount = imageData.numLevels;
        //    descriptor.dimension = gfx::TextureDimension::TEXTURE_2D;
        //    descriptor.debugName = imageData.debugName;
        //};
        //auto texture = Engine::GetGPUDevice()->CreateTexture(descriptor);

        //gfx::BufferDescriptor bufferDescriptor;
        //{
        //    bufferDescriptor.size = imageData.byteLength;
        //    bufferDescriptor.usage = gfx::BufferUsage::COPY_DST | gfx::BufferUsage::COPY_SRC;
        //}

        //auto buffer = Engine::GetGPUDevice()->CreateBuffer(bufferDescriptor);
        //buffer->SetSubData(0, imageData.byteLength, imageData.bytes);

        //auto commandEncoder = Engine::GetGPUDevice()->CreateCommandEncoder();

        //for (uint32 face = 0; face < imageData.numFaces; ++face)
        //{
        //    for (uint32_t level = 0; level < descriptor.mipLevelCount; level++)
        //    {
        //        // Calculate offset into staging buffer for the current mip level and face
        //        uint32 offset = imageData.GetOffset(0, face, level);

        //        gfx::BufferCopyView bufferCopyView;
        //        {
        //            bufferCopyView.buffer = buffer;
        //            bufferCopyView.offset = offset;
        //            bufferCopyView.rowsPerImage = imageData.baseHeight >> level;
        //            bufferCopyView.bytesPerRow = (imageData.baseWidth >> level) * 4;
        //        }

        //        gfx::TextureCopyView textureCopyView;
        //        {
        //            textureCopyView.texture = texture;
        //            textureCopyView.origin = { 0, 0, 0 };
        //            textureCopyView.mipLevel = level;
        //            textureCopyView.baseArrayLayer = face;
        //            textureCopyView.arrayLayerCount = 1;
        //        }

        //        auto size = gfx::Extent3D{ imageData.baseHeight >> level, imageData.baseWidth >> level, 1 };
        //        commandEncoder->CopyBufferToTexture(bufferCopyView, textureCopyView, size);
        //    }
        //}

        //auto cmdBuffer = commandEncoder->Finish();
        //Engine::GetGPUDevice()->GetQueue()->Submit(1, &cmdBuffer);

        //return texture;
    }

    TexturePtr LoadTextureFromData(uint32 texWidth, uint32 texHeight, uint32 texChannels, const std::uint8_t* pixels, std::uint32_t byteLength, const std::string& debugName)
    {
		return std::make_shared<RxImage>(texWidth, texHeight, texChannels, pixels);

        //ImageData imageData;
        //{
        //    imageData.debugName = debugName;
        //    imageData.baseWidth = texWidth;
        //    imageData.baseHeight = texHeight;
        //    imageData.byteLength = texWidth * texHeight * 4;
        //    imageData.format = TextureFormat::RGBA8UNORM;
        //    imageData.numFaces = 1;
        //    imageData.numLevels = GetMipMapLevels(texWidth);
        //}
        ////imageData.SetOffset(0, 0, 0, 0);
        //std::vector<uint8> bytes;

        //std::vector<uint8> tmpData;
        //const uint8* pData = nullptr;

        //if (texChannels != 4)
        //{
        //    tmpData.resize(texWidth * texHeight * 4, 255);
        //    uint8* pCurrent = tmpData.data();
        //    for (int i = 0; i < texWidth * texHeight * texChannels; i += texChannels)
        //    {
        //        for (int j = 0; j < texChannels; ++j)
        //        {
        //            pCurrent[j] = pixels[i + j];
        //        }
        //        pCurrent += 4;
        //    }
        //    pData = tmpData.data();
        //}
        //else
        //{
        //    pData = pixels;
        //}

        //std::vector<uint8> buffer;
        //buffer.reserve(texWidth * texHeight * texChannels * 2);
        //texChannels = 4;
        //auto mipmaps = GenerateMipMap(texWidth, texHeight, pData, texChannels);
        //for (uint32 level = 0; level < mipmaps.size(); ++level)
        //{
        //    auto w = texWidth >> level;
        //    auto h = texHeight >> level;
        //    auto size = w * h * texChannels;
        //    auto offset = buffer.size();
        //    buffer.resize(buffer.size() + size);
        //    memcpy(buffer.data() + offset, mipmaps[level].data(), size);
        //    imageData.SetOffset(0, 0, level, offset);
        //}

        //imageData.bytes = buffer.data();
        //imageData.byteLength = buffer.size();

        //auto texture = CreateCubeTexture(imageData);

        //return texture;
    }

    TexturePtr LoadTextureFromData(const std::uint8_t* data, std::uint32_t byteLength, const std::string& fileName)
    {
		Assert(false);
		return {};
        /*int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load_from_memory(data, byteLength, &texWidth, &texHeight, &texChannels, STBI_default);
        Assert(pixels != nullptr);

        auto texture = LoadTextureFromData(texWidth, texHeight, texChannels, pixels, texWidth * texHeight * texChannels, fileName);
        STBI_FREE(pixels);

        return texture;*/
    }

    TexturePtr LoadTextureFromKtxFormat(const void* bytes, uint32 size, const std::string& fileName = "")
    {
		Assert(false);
		return {};
        //ktxTexture* ktxTexture;
        //KTX_error_code result = ktxTexture_CreateFromMemory((ktx_uint8_t*)bytes, size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
        //Assert(result == KTX_SUCCESS);

        //TexturePtr texture;

        //Assert(ktxTexture->glFormat == 6408);
        //ImageData imageData;
        //{
        //    imageData.debugName = fileName;
        //    imageData.format = gfx::TextureFormat::RGBA8UNORM;
        //    imageData.baseWidth = ktxTexture->baseWidth;
        //    imageData.baseHeight = ktxTexture->baseHeight;
        //    imageData.numLevels = ktxTexture->numLevels;
        //    imageData.numFaces = ktxTexture->numFaces;
        //    imageData.bytes = ktxTexture_GetData(ktxTexture);
        //    imageData.byteLength = ktxTexture_GetSize(ktxTexture);
        //}

        //for (uint32 face = 0; face < imageData.numFaces; ++face)
        //{
        //    for (uint32_t level = 0; level < ktxTexture->numLevels; level++)
        //    {
        //        // Calculate offset into staging buffer for the current mip level and face
        //        ktx_size_t offset;
        //        KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, level, 0, face, &offset);
        //        imageData.SetOffset(0, face, level, offset);
        //    }
        //}
        //texture = CreateCubeTexture(imageData);

        //if (ktxTexture)
        //{
        //    ktxTexture_Destroy(ktxTexture);
        //}

        //return texture;
    }

    TexturePtr LoadTextureFromUri(const std::string& filename)
    {
        const TData& imageData = FileSystem::GetInstance()->GetBinaryData(filename.c_str());

        if (filename.substr(filename.find_last_of(".") + 1) == "ktx")
        {
            return LoadTextureFromKtxFormat(imageData.data(), imageData.size(), filename);
        }
        else
        {
            return LoadTextureFromData(imageData.data(), imageData.size(), filename);
        }
    }

    TexturePtr LoadCubeTexture(const std::string& cubeTextureName)
    {
		Assert(false);
		return {};
        //constexpr uint32 kFaceNum = 6;
        //const char* kFaceNames[kFaceNum] = { "px", "nx", "py", "ny", "pz", "nz" };
        //std::string fileExt = "";
        //for (const auto& ext : sImageExts)
        //{
        //    std::string filename = "Textures\\CubeTextures\\" + cubeTextureName + "\\" + "nx" + ext;
        //    if (!FileSystem::GetInstance()->GetAbsFilePath(filename.c_str()).empty())
        //    {
        //        fileExt = ext;
        //        break;
        //    }
        //}

        //Assert(!fileExt.empty());

        //ImageData imageData;
        //{
        //    imageData.debugName = cubeTextureName;
        //    imageData.format = gfx::TextureFormat::RGBA8UNORM;
        //    imageData.numFaces = kFaceNum;
        //    imageData.numLevels = 1;
        //}
        //std::vector<uint8> bytes;

        //for (int face = 0; face < kFaceNum; ++face)
        //{
        //    std::string filename = "Textures\\CubeTextures\\" + cubeTextureName + "\\" + kFaceNames[face] + fileExt;
        //    const TData& data = FileSystem::GetInstance()->GetBinaryData(filename.c_str());

        //    int texWidth, texHeight, texChannels;
        //    std::vector<uint8> tmpData;
        //    const uint8* pData = nullptr;
        //    stbi_uc* pixels = stbi_load_from_memory(data.data(), data.size(), &texWidth, &texHeight, &texChannels, STBI_default);
        //    pData = pixels;
        //    if (texChannels != 4)
        //    {
        //        tmpData.resize(texWidth * texHeight * 4, 255);
        //        pData = tmpData.data();
        //        uint8* pCurrent = tmpData.data();
        //        for (int i = 0; i < texWidth * texHeight * texChannels; i += texChannels)
        //        {
        //            for (int j = 0; j < texChannels; ++j)
        //            {
        //                pCurrent[j] = pixels[i + j];
        //            }
        //            pCurrent += 4;
        //        }
        //    }
        //    /*uint32 offset = bytes.size();
        //    imageData.SetOffset(0, face, 0, offset);*/
        //    uint32 length = texWidth * texHeight * 4;
        //    if (face == 0)
        //    {
        //        imageData.baseWidth = texWidth;
        //        imageData.baseHeight = texHeight;
        //        //imageData.byteLength = texWidth * texHeight * 4 * kFaceNum;
        //        bytes.reserve(texWidth * texHeight * 4 * kFaceNum * 2);
        //    }
        //    Assert(imageData.baseWidth == texWidth);
        //    Assert(imageData.baseHeight == texHeight);

        //    auto mipmaps = GenerateMipMap(texWidth, texHeight, pData, texChannels);
        //    for (uint32 level = 0; level < mipmaps.size(); ++level)
        //    {
        //        auto w = texWidth >> level;
        //        auto h = texHeight >> level;
        //        auto size = w * h * texChannels;
        //        auto offset = bytes.size();
        //        bytes.resize(bytes.size() + size);
        //        memcpy(bytes.data() + offset, mipmaps[level].data(), size);
        //        imageData.SetOffset(0, face, level, offset);
        //    }


        //    //bytes.resize(offset + length);
        //    //memcpy(bytes.data() + offset, pData, length);

        //    STBI_FREE(pixels);
        //}
        //imageData.numLevels = GetMipMapLevels(imageData.baseWidth);
        //imageData.bytes = bytes.data();
        //imageData.byteLength = bytes.size();

        //auto texture = CreateCubeTexture(imageData);
        //return texture;
    }

    // Translation of Ken Perlin's JAVA implementation (http://mrl.nyu.edu/~perlin/noise/)
    template <typename T>
    class PerlinNoise
    {
    private:
        uint32_t permutations[512];
        T fade(T t)
        {
            return t * t * t * (t * (t * (T)6 - (T)15) + (T)10);
        }
        T lerp(T t, T a, T b)
        {
            return a + t * (b - a);
        }
        T grad(int hash, T x, T y, T z)
        {
            // Convert LO 4 bits of hash code into 12 gradient directions
            int h = hash & 15;
            T u = h < 8 ? x : y;
            T v = h < 4 ? y : h == 12 || h == 14 ? x : z;
            return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
        }
    public:
        PerlinNoise()
        {
            // Generate random lookup for permutations containing all numbers from 0..255
            std::vector<uint8_t> plookup;
            plookup.resize(256);
            std::iota(plookup.begin(), plookup.end(), 0);
            std::default_random_engine rndEngine(std::random_device{}());
            std::shuffle(plookup.begin(), plookup.end(), rndEngine);

            for (uint32_t i = 0; i < 256; i++)
            {
                permutations[i] = permutations[256 + i] = plookup[i];
            }
        }
        T noise(T x, T y, T z)
        {
            // Find unit cube that contains point
            int32_t X = (int32_t)floor(x) & 255;
            int32_t Y = (int32_t)floor(y) & 255;
            int32_t Z = (int32_t)floor(z) & 255;
            // Find relative x,y,z of point in cube
            x -= floor(x);
            y -= floor(y);
            z -= floor(z);

            // Compute fade curves for each of x,y,z
            T u = fade(x);
            T v = fade(y);
            T w = fade(z);

            // Hash coordinates of the 8 cube corners
            uint32_t A = permutations[X] + Y;
            uint32_t AA = permutations[A] + Z;
            uint32_t AB = permutations[A + 1] + Z;
            uint32_t B = permutations[X + 1] + Y;
            uint32_t BA = permutations[B] + Z;
            uint32_t BB = permutations[B + 1] + Z;

            // And add blended results for 8 corners of the cube;
            T res = lerp(w, lerp(v,
                lerp(u, grad(permutations[AA], x, y, z), grad(permutations[BA], x - 1, y, z)), lerp(u, grad(permutations[AB], x, y - 1, z), grad(permutations[BB], x - 1, y - 1, z))),
                lerp(v, lerp(u, grad(permutations[AA + 1], x, y, z - 1), grad(permutations[BA + 1], x - 1, y, z - 1)), lerp(u, grad(permutations[AB + 1], x, y - 1, z - 1), grad(permutations[BB + 1], x - 1, y - 1, z - 1))));
            return res;
        }
    };

    // Fractal noise generator based on perlin noise above
    template <typename T>
    class FractalNoise
    {
    private:
        PerlinNoise<float> perlinNoise;
        uint32_t octaves;
        T frequency;
        T amplitude;
        T persistence;
    public:

        FractalNoise(const PerlinNoise<T>& perlinNoise)
        {
            this->perlinNoise = perlinNoise;
            octaves = 6;
            persistence = (T)0.5;
        }

        T noise(T x, T y, T z)
        {
            T sum = 0;
            T frequency = (T)1;
            T amplitude = (T)1;
            T max = (T)0;
            for (uint32_t i = 0; i < octaves; i++)
            {
                sum += perlinNoise.noise(x * frequency, y * frequency, z * frequency) * amplitude;
                max += amplitude;
                amplitude *= persistence;
                frequency *= (T)2;
            }

            sum = sum / max;
            return (sum + (T)1.0) / (T)2.0;
        }
    };


    void Gen3dNoiseData(uint32 width, uint32 height, uint32 depth, std::vector<float>& buffer)
    {
//        auto bufferSize = width * height * depth;
//
//        buffer.resize(bufferSize);
//
//        std::string filename = "3dNoiseTextures/" + std::to_string(width) + "x" + std::to_string(height) + "x" + std::to_string(depth) + ".tex";
//
//        // try to load from files
//        {
//            TData data;
//            if (FileSystem::ReadBinaryData(filename.c_str(), data) && data.size() == buffer.size() * sizeof(float))
//            {
//                memcpy(buffer.data(), data.data(), data.size());
//                return;
//            }
//        }
//        
//
//        float* data = buffer.data();
//
//        PerlinNoise<float> perlinNoise;
//        FractalNoise<float> fractalNoise(perlinNoise);
//
//        const float noiseScale = static_cast<float>(rand() % 10) + 4.0f;
//
//        // Generate perlin based noise
//        std::cout << "Generating " << width << " x " << height << " x " << depth << " noise texture..." << std::endl;
//
//        auto tStart = std::chrono::high_resolution_clock::now();
//
//#pragma omp parallel for
//        for (int32_t z = 0; z < depth; z++)
//        {
//            for (int32_t y = 0; y < height; y++)
//            {
//                for (int32_t x = 0; x < width; x++)
//                {
//                    float nx = (float)x / (float)width;
//                    float ny = (float)y / (float)height;
//                    float nz = (float)z / (float)depth;
//#define FRACTAL
//#ifdef FRACTAL
//                    float n = fractalNoise.noise(nx * noiseScale, ny * noiseScale, nz * noiseScale);
//#else
//                    float n = 20.0 * perlinNoise.noise(nx, ny, nz);
//#endif
//                    n = n - floor(n);
//
//                    data[x + y * width + z * width * height] = n;
//                }
//            }
//        }
//
//        auto tEnd = std::chrono::high_resolution_clock::now();
//        auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
//
//        std::cout << "Done in " << tDiff << "ms" << std::endl;
//
//        FileSystem::WriteBinaryData(filename.c_str(), data, buffer.size() * sizeof(float));
    }


    TexturePtr Create3DFloatTexture(uint32 width, uint32 height, uint32 depth)
    {
		Assert(false);
		return {};
        /*std::vector<float> noiseTextureData;
        Gen3dNoiseData(width, height, depth, noiseTextureData);
        size_t bufferSize = noiseTextureData.size() * sizeof(noiseTextureData[0]);

        gfx::TextureDescriptor descriptor;
        {
            descriptor.sampleCount = 1;
            descriptor.format = gfx::TextureFormat::R32FLOAT;
            descriptor.usage = gfx::TextureUsage::SAMPLED | gfx::TextureUsage::COPY_DST;
            descriptor.size = { width, height, depth };
            descriptor.arrayLayerCount = 1;
            descriptor.mipLevelCount = 1;
            descriptor.dimension = gfx::TextureDimension::TEXTURE_3D;
            descriptor.debugName = "3dNoiseMap";
        };
        auto texture = Engine::GetGPUDevice()->CreateTexture(descriptor);


        gfx::BufferDescriptor bufferDescriptor;
        {
            bufferDescriptor.size = bufferSize;
            bufferDescriptor.usage = gfx::BufferUsage::COPY_DST | gfx::BufferUsage::COPY_SRC;
        }

        auto buffer = Engine::GetGPUDevice()->CreateBuffer(bufferDescriptor);
        buffer->SetSubData(0, bufferSize, noiseTextureData.data());

        auto sliceImageSize = width * height * sizeof(float);
        auto commandEncoder = Engine::GetGPUDevice()->CreateCommandEncoder();

        for (int z = 0; z < depth; z++)
        {
            uint32 offset = sliceImageSize * z;

            gfx::BufferCopyView bufferCopyView;
            {
                bufferCopyView.buffer = buffer;
                bufferCopyView.offset = offset;
                bufferCopyView.rowsPerImage = height;
                bufferCopyView.bytesPerRow = width * sizeof(float);
            }

            gfx::TextureCopyView textureCopyView;
            {
                textureCopyView.texture = texture;
                textureCopyView.origin = { 0, 0, z };
                textureCopyView.mipLevel = 0;
                textureCopyView.baseArrayLayer = 0;
                textureCopyView.arrayLayerCount = 1;
            }

            auto size = gfx::Extent3D{ width, height, 1 };
            commandEncoder->CopyBufferToTexture(bufferCopyView, textureCopyView, size);
        }

        auto cmdBuffer = commandEncoder->Finish();
        Engine::GetGPUDevice()->GetQueue()->Submit(1, &cmdBuffer);

        return texture;*/
    }

}// end of namespace