#pragma once

#include "Raytracer.h"


class PhotonMapper: public Raytracer
{
public:
	PhotonMapper(const RTContext& context);

	virtual void RenderAsync() noexcept;

protected:
	void EmitPhoton(const RxRay& ray, vec4 flux, const LightData& light) noexcept;
};