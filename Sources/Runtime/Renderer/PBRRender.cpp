
#include "PBRRender.h"
#include "Graphics/RxSampler.h"
#include <algorithm>
#include "ShadingBase.h"


/*

- Sampling the GGX Distribution of Visible Normals
  http://jcgt.org/published/0007/04/01/

- Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs
  http://jcgt.org/published/0003/02/03/

- Microfacet Models for Refraction through Rough Surfaces
  https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.html

*/

static vec2 _SampleSphericalMap(vec3 v)
{
	static const vec2 invAtan = vec2(0.1591f, 0.3183f);
	vec2			  uv	  = vec2(glm::atan(v.z, v.x), asin(v.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

Color4f PBRRender::RenderSkybox(const GlobalConstantBuffer& cGlobalBuffer, Vector3f worldPosition) noexcept
{
#if 1
	glm::vec4 viewPos	 = cGlobalBuffer.ViewMatrix * glm::vec4(worldPosition, 1.0f);
	glm::vec3 viewCoords = viewPos.xyz;
	viewCoords			 = viewCoords / viewPos.w;

	glm::mat4 skyboxViewMatrix = glm::mat4(glm::mat3(cGlobalBuffer.ViewMatrix));
	glm::vec3 skyboxCoords	   = glm::vec3(skyboxViewMatrix * glm::vec4(viewCoords, 1.0f));

	// return { 0.2f, 0.2f, 0, 1 };
	vec3 color = textureCubeLod(mEnvTexture.get(), worldPosition.xyz, 0).rgb;
	// color	   = glm::pow(color, vec3(2.2f));

	//	// HDR tonemapping
	// color = color / (color + vec3(1.0));
	//// gamma correct
	// color = pow(color, vec3(1.0 / 2.2));

	auto FragColor = vec4(color, 1.0);
	return FragColor;
#else
	vec2 uv = _SampleSphericalMap(glm::normalize(worldPosition));
	return texture2D(mSphericalEnvTexture, vec2(uv.x, 1.0f - uv.y), 0);
#endif


	// return Color4f(prefilteredColor, 1.0f);
	// return Color4f{ 0, (worldPosition.y + 1000.0f) / 2000.0f, (worldPosition.z + 1000.0f)/ 2000.0f, 1.0f };
}