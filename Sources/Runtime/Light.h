#pragma once

#include <sys/platform.h>
#include <sys/ref.h>
#include <sys/alloc.h>
#include <math/emath.h>
#include <math/vec3.h>
#include <math/emath.h>
#include <math/affinespace.h>

#include "Types.h"

enum LightType
{
	LIGHT_AMBIENT,
	LIGHT_POINT,
	LIGHT_DIRECTIONAL,
	LIGHT_SPOT,
	LIGHT_DISTANT,
	LIGHT_TRIANGLE,
	LIGHT_QUAD,
};

class Light
{
	ALIGNED_CLASS(16)

public:
	Light(LightType type)
		: type(type)
	{
	}

	LightType getType() const
	{
		return type;
	}

private:
	LightType type;
};

class AmbientLight : public Light
{
public:
	AmbientLight(const Vector3f& L)
		: Light(LIGHT_AMBIENT)
		, L(L)
	{
	}

	AmbientLight transform(const embree::AffineSpace3fa& space) const
	{
		return AmbientLight(L);
	}

	static AmbientLight lerp(const AmbientLight& light0, const AmbientLight& light1, const float f)
	{
		return AmbientLight(glm::lerp(light0.L, light1.L, f));
	}

public:
	Vector3f L; //!< radiance of ambient light
};

class PointLight : public Light
{
public:
	PointLight(const Vector3f& P, const Vector3f& I)
		: Light(LIGHT_POINT)
		, P(P)
		, I(I)
	{
	}

	PointLight transform(const embree::AffineSpace3fa& space) const
	{
		auto p = embree::xfmPoint(space, { P.x, P.y, P.z });
		return PointLight({p.x, p.y, p.z}, I);
	}

	static PointLight lerp(const PointLight& light0, const PointLight& light1, const float f)
	{
		return PointLight(glm::lerp(light0.P, light1.P, f),
						  glm::lerp(light0.I, light1.I, f));
	}

public:
	Vector3f P; //!< position of point light
	Vector3f I; //!< radiant intensity of point light
};

class DirectionalLight : public Light
{
public:
	DirectionalLight(const Vector3f& D, const Vector3f& E)
		: Light(LIGHT_DIRECTIONAL)
		, D(D)
		, E(E)
	{
	}

	DirectionalLight transform(const embree::AffineSpace3fa& space) const
	{
		auto p = embree::xfmVector(space, { D.x, D.y, D.z });
		return DirectionalLight({p.x, p.y, p.z}, E);
	}

	static DirectionalLight lerp(const DirectionalLight& light0, const DirectionalLight& light1, const float f)
	{
		return DirectionalLight(glm::lerp(light0.D, light1.D, f),
								glm::lerp(light0.E, light1.E, f));
	}

public:
	Vector3f D; //!< Light direction
	Vector3f E; //!< Irradiance (W/m^2)
};

class SpotLight : public Light
{
public:
	SpotLight(const Vector3f& P, const Vector3f& D, const Vector3f& I, float angleMin, float angleMax)
		: Light(LIGHT_SPOT)
		, P(P)
		, D(D)
		, I(I)
		, angleMin(angleMin)
		, angleMax(angleMax)
	{
	}

	SpotLight transform(const embree::AffineSpace3fa& space) const
	{
		auto p = embree::xfmPoint(space, { P.x, P.y, P.z });
		auto d = embree::xfmVector(space, { D.x, D.y, D.z });
		return SpotLight({ p.x, p.y, p.z }, {d.x, d.y, d.z}, I, angleMin, angleMax);
	}

	static SpotLight lerp(const SpotLight& light0, const SpotLight& light1, const float f)
	{
		return SpotLight(glm::lerp(light0.P, light1.P, f),
						 glm::lerp(light0.D, light1.D, f),
						 glm::lerp(light0.I, light1.I, f),
						 glm::lerp(light0.angleMin, light1.angleMin, f),
						 glm::lerp(light0.angleMax, light1.angleMax, f));
	}

public:
	Vector3f P;				   //!< Position of the spot light
	Vector3f D;				   //!< Light direction of the spot light
	Vector3f I;				   //!< Radiant intensity (W/sr)
	float  angleMin, angleMax; //!< Linear falloff region
};

class DistantLight : public Light
{
public:
	DistantLight(const Vector3f& D, const Vector3f& L, const float halfAngle)
		: Light(LIGHT_DISTANT)
		, D(D)
		, L(L)
		, halfAngle(halfAngle)
		, radHalfAngle(embree::deg2rad(halfAngle))
		, cosHalfAngle(embree::cos(embree::deg2rad(halfAngle)))
	{
	}

	DistantLight transform(const embree::AffineSpace3fa& space) const
	{
		auto d = xfmVector(space, { D.x, D.y, D.z });
		return DistantLight({d.x, d.y, d.z}, L, halfAngle);
	}

	static DistantLight lerp(const DistantLight& light0, const DistantLight& light1, const float f)
	{
		return DistantLight(glm::lerp(light0.D, light1.D, f),
							glm::lerp(light0.L, light1.L, f),
							glm::lerp(light0.halfAngle, light1.halfAngle, f));
	}

public:
	Vector3f D;			 //!< Light direction
	Vector3f L;			 //!< Radiance (W/(m^2*sr))
	float  halfAngle;	 //!< Half illumination angle
	float  radHalfAngle; //!< Half illumination angle in radians
	float  cosHalfAngle; //!< Cosine of half illumination angle
};

class TriangleLight : public Light
{
public:
	TriangleLight(const Vector3f& v0, const Vector3f& v1, const Vector3f& v2, const Vector3f& L)
		: Light(LIGHT_TRIANGLE)
		, v0(v0)
		, v1(v1)
		, v2(v2)
		, L(L)
	{
	}

	TriangleLight transform(const embree::AffineSpace3fa& space) const
	{
		auto vv0 = embree::xfmPoint(space, {v0.x, v0.y, v0.z});
		auto vv1 = embree::xfmPoint(space, {v1.x, v1.y, v1.z});
		auto vv2 = embree::xfmPoint(space, {v2.x, v2.y, v2.z});

		return TriangleLight({ vv0.x, vv0.y, vv0.z }, { vv1.x, vv1.y, vv1.z }, { vv2.x, vv2.y, vv2.z }, L);
	}

	static TriangleLight lerp(const TriangleLight& light0, const TriangleLight& light1, const float f)
	{
		return TriangleLight(glm::lerp(light0.v0, light1.v0, f),
							 glm::lerp(light0.v1, light1.v1, f),
							 glm::lerp(light0.v2, light1.v2, f),
							 glm::lerp(light0.L, light1.L, f));
	}

public:
	Vector3f v0;
	Vector3f v1;
	Vector3f v2;
	Vector3f L;
};

class QuadLight : public Light
{
public:
	QuadLight(const Vector3f& v0, const Vector3f& v1, const Vector3f& v2, const Vector3f& v3, const Vector3f& L)
		: Light(LIGHT_QUAD)
		, v0(v0)
		, v1(v1)
		, v2(v2)
		, v3(v3)
		, L(L)
	{
	}

	QuadLight transform(const embree::AffineSpace3fa& space) const
	{
		auto vv0 = embree::xfmPoint(space, { v0.x, v0.y, v0.z });
		auto vv1 = embree::xfmPoint(space, { v1.x, v1.y, v1.z });
		auto vv2 = embree::xfmPoint(space, { v2.x, v2.y, v2.z });
		auto vv3 = embree::xfmPoint(space, { v3.x, v3.y, v3.z });
		return QuadLight({ vv0.x, vv0.y, vv0.z }, { vv1.x, vv1.y, vv1.z }, { vv2.x, vv2.y, vv2.z }, { vv3.x, vv3.y, vv3.z }, L);
	}

	static QuadLight lerp(const QuadLight& light0, const QuadLight& light1, const float f)
	{
		return QuadLight(glm::lerp(light0.v0, light1.v0, f),
						 glm::lerp(light0.v1, light1.v1, f),
						 glm::lerp(light0.v2, light1.v2, f),
						 glm::lerp(light0.v3, light1.v3, f),
						 glm::lerp(light0.L, light1.L, f));
	}

public:
	Vector3f v0;
	Vector3f v1;
	Vector3f v2;
	Vector3f v3;
	Vector3f L;
};