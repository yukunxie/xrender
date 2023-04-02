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

Color4f RxSampler::SamplePixel(const RxImage* image, float u, float v) const
{
	int	  w	 = image->GetWidth();
	int	  h	 = image->GetHeight();
	int	  pw = int(u * w);
	int	  ph = int(v * h);
	float fw = u * w - pw;
	float fh = v * h - ph;

	Vector2I texCoord = GetWrappedWithRepeat({ pw, ph }, { w, h });

	Color3B color = image->SamplePixel(texCoord);
	return (1.0f / 255) * Color4f(Color4B(color, 255));
}