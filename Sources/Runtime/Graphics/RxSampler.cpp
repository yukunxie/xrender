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

Color4f RxSampler::ReadPixel(const Texture* texture, float u, float v, int lod) const
{
	const auto& image = ((const Texture2D*)texture)->GetImageWithMip(lod);
	int			w	  = (image->GetWidth() - 1);
	int			h	  = (image->GetHeight() - 1);
	int			pw	  = int(u * w);
	int			ph	  = int(v * h);
	float		fx	  = u * w - pw;
	float		fy	  = v * h - ph;

	Color4f c00, c10, c01, c11;
	{
		Vector2I texCoord = GetWrappedWithRepeat({ pw, ph }, { image->GetWidth(), image->GetHeight() });
		c00				  = image->ReadPixel(texCoord.x, texCoord.y);
	}
	{
		Vector2I texCoord = GetWrappedWithRepeat({ pw + 1, ph }, { image->GetWidth(), image->GetHeight() });
		c10				  = image->ReadPixel(texCoord.x, texCoord.y);
	}
	{
		Vector2I texCoord = GetWrappedWithRepeat({ pw, ph + 1 }, { image->GetWidth(), image->GetHeight() });
		c01				  = image->ReadPixel(texCoord.x, texCoord.y);
	}
	{
		Vector2I texCoord = GetWrappedWithRepeat({ pw + 1, ph + 1 }, { image->GetWidth(), image->GetHeight() });
		c11				  = image->ReadPixel(texCoord.x, texCoord.y);
	}
	Color4f c0 = glm::mix(c00, c10, fx);
	Color4f c1 = glm::mix(c01, c11, fx);
	Color4f c  = glm::mix(c0, c1, fy);

	return c;
}

Color4f texture2D(const Texture* texture, Vector2f uv, float lod)
{
	static RxSampler* sampler = RxSampler::CreateSampler();
	int				  lod0	  = int(glm::floor(lod));
	int				  lod1	  = int(glm::ceil(lod));
	Color4f			  c0	  = sampler->ReadPixel(texture, uv, lod0);
	Color4f			  c1	  = sampler->ReadPixel(texture, uv, lod1);

	return glm::mix(c0, c1, glm::fract(lod));
}

Color4f textureCube(const Texture* texture, Vector3f dir, float lod)
{
	return textureCubeLod(texture, dir, lod);
}

int convert_xyz_to_cube_uv(vec3 worldPosition, vec2& uv)
{
	int		  face	 = 0;
	glm::vec3 absPos = glm::abs(worldPosition);
	if (absPos.x >= absPos.y && absPos.x >= absPos.z)
	{
		if (worldPosition.x > 0)
		{
			face = 0;
			uv	 = glm::vec2(-worldPosition.z, -worldPosition.y);
		}
		else
		{
			face = 1;
			uv	 = glm::vec2(worldPosition.z, -worldPosition.y);
		}
	}
	else if (absPos.y >= absPos.x && absPos.y >= absPos.z)
	{
		if (worldPosition.y > 0)
		{
			face = 2;
			uv	 = glm::vec2(-worldPosition.x, -worldPosition.z);
		}
		else
		{
			face = 3;
			uv	 = glm::vec2(-worldPosition.x, worldPosition.z);
		}
	}
	else
	{
		if (worldPosition.z > 0)
		{
			face = 4;
			uv	 = glm::vec2(worldPosition.x, -worldPosition.y);
		}
		else
		{
			face = 5;
			uv	 = glm::vec2(-worldPosition.x, -worldPosition.y);
		}
	}

	float ma	   = glm::max(absPos.x, absPos.y);
	float maxValue = glm::max(ma, absPos.z);

	uv = (uv / maxValue) * 0.5f + 0.5f;

	return face;
}

Color4f Lookup(const Texture* texture, Vector3f dir, float lod)
{
	dir = glm::normalize(dir);
	vec2 uv;
	int	 face = convert_xyz_to_cube_uv(dir, uv);

	const auto&		  faceTexture = ((const TextureCube*)texture)->GetFace(face);
	static RxSampler* sampler	  = RxSampler::CreateSampler();

	int		lod0 = int(glm::floor(lod));
	int		lod1 = int(glm::ceil(lod));
	Color4f c0	 = sampler->ReadPixel(faceTexture.get(), uv, lod0);
	Color4f c1	 = sampler->ReadPixel(faceTexture.get(), uv, lod1);

	return glm::mix(c0, c1, glm::fract(lod));
}

Color4f textureCubeLod(const Texture* texture, Vector3f dir, float lod)
{
	vec2 uv;
	int	 face = convert_xyz_to_cube_uv(dir, uv);

	const auto&		  faceTexture = ((const TextureCube*)texture)->GetFace(face);
	static RxSampler* sampler	  = RxSampler::CreateSampler();

	int		lod0 = std::max(0, int(glm::floor(lod)));
	int		lod1 = std::max(0, int(glm::ceil(lod)));

	Color4f c0	 = sampler->ReadPixel(faceTexture.get(), uv, lod0);
	Color4f c1	 = sampler->ReadPixel(faceTexture.get(), uv, lod1);

	return glm::mix(c0, c1, glm::fract(lod));
}