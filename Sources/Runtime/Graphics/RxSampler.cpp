#include "RxSampler.h"

RxSampler::RxSampler(RxWrapMode wrapModel)
	: mWrapModel(wrapModel)
{
}

RxSampler::~RxSampler()
{
}

RxSampler* RxSampler::CreateSampler(RxSamplerType type, RxWrapMode wrapModel)
{
	return new RxSampler(wrapModel);
}


Vector2I GetWrappedWithRepeat(Vector2I pos, Extent2D size)
{
	return { pos.x % size.x, pos.y % size.y };
}

Color4f RxSampler::ReadPixel(const PhysicalImage* image, float u, float v) const
{
	int	  w	 = image->GetWidth();
	int	  h	 = image->GetHeight();
	int	  pw = int(u * w);
	int	  ph = int(v * h);
	float fw = u * w - pw;
	float fh = v * h - ph;

	Vector2I texCoord = GetWrappedWithRepeat({ pw, ph }, { w, h });

	Color4f color = image->ReadPixel(texCoord.x, texCoord.y);
	return color;
}

Color4f texture2D(const PhysicalImage* texture, Vector2f uv)
{
	static RxSampler* sampler = RxSampler::CreateSampler();
	return sampler->ReadPixel(texture, uv);
}

Color4f textureCube(const PhysicalImage* texture, Vector3f dir, int Lod)
{
	return ((RxImageCube*)texture)->ReadPixel(dir);
}

Color4f textureCubeLod(const PhysicalImage* texture, Vector3f dir, int Lod)
{
	return textureCube(texture, dir, Lod);
}