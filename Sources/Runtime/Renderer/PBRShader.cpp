#include "PBRShader.h"
#include "Graphics/RxSampler.h"

#include <algorithm>

#include "ShadingBase.h"

//// ----------------------------------------------------------------------------
//float DistributionGGX(vec3 N, vec3 H, float roughness)
//{
//	float a		 = roughness * roughness;
//	float a2	 = a * a;
//	float NdotH	 = max(dot(N, H), 0.0f);
//	float NdotH2 = NdotH * NdotH;
//
//	float nom	= a2;
//	float denom = (NdotH2 * (a2 - 1.0) + 1.0f);
//	denom		= PI * denom * denom;
//
//	return nom / denom;
//}
//// ----------------------------------------------------------------------------
//float GeometrySchlickGGX(float NdotV, float roughness)
//{
//	float r = (roughness + 1.0f);
//	float k = (r * r) / 8.0f;
//
//	float nom	= NdotV;
//	float denom = NdotV * (1.0f - k) + k;
//
//	return nom / denom;
//}
//// ----------------------------------------------------------------------------
//float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
//{
//	float NdotV = max(dot(N, V), 0.0f);
//	float NdotL = max(dot(N, L), 0.0f);
//	float ggx2	= GeometrySchlickGGX(NdotV, roughness);
//	float ggx1	= GeometrySchlickGGX(NdotL, roughness);
//
//	return ggx1 * ggx2;
//}
//// ----------------------------------------------------------------------------
//vec3 F_Schlick(float cosTheta, vec3 F0)
//{
//	return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
//}
//// ----------------------------------------------------------------------------
//vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
//{
//	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
//} 
//
//
//static vec2 _SampleSphericalMap(vec3 v)
//{
//	static const vec2 invAtan = vec2(0.1591f, 0.3183f);
//	vec2			  uv	  = vec2(glm::atan(v.z, v.x), asin(v.y));
//	uv *= invAtan;
//	uv += 0.5;
//	return uv;
//}

vec4 CalcIrradiance(TexturePtr environmentMap, vec3 WorldPos)
{
	vec3 N = normalize(WorldPos);

	vec3 irradiance = vec3(0.0);

	// tangent space calculation from origin point
	vec3 up	   = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, N));
	up		   = normalize(cross(N, right));

	float sampleDelta = 0.1;
	float nrSamples	  = 0.0;
	for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
	{
		for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
		{
			// spherical to cartesian (in tangent space)
			vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			// tangent space to world
			vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

			irradiance += texture2D(environmentMap, SampleSphericalMap(sampleVec)).rgb * cos(theta) * sin(theta);
			nrSamples++;
		}
	}
	irradiance = PI * irradiance * (1.0f / float(nrSamples));

	return vec4(irradiance, 1.0);
}

// https://learnopengl.com/PBR/Lighting
// https://learnopengl.com/code_viewer_gh.php?code=src/6.pbr/1.1.lighting/1.1.pbr.fs
Color4f PBRShading(const GlobalConstantBuffer& cGlobalBuffer, const GBufferData& gBufferData, const EnvironmentTextures& gEnvironmentData)
{
	vec3  N			= normalize(gBufferData.WorldNormal);
	vec3  V			= normalize(cGlobalBuffer.EyePos.xyz - gBufferData.Position);
	vec3  R			= reflect(-V, N); 
	vec3  albedo	= pow(gBufferData.Albedo, vec3(2.2f));
	float metallic	= gBufferData.Material.r;
	float roughness = gBufferData.Material.g;
	vec3  ao		= gBufferData.AOMask;
	vec4  FragColor(0, 0, 0, 1);

	// calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
	// of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
	vec3 F0 = vec3(0.04);
	F0		= mix(F0, albedo, metallic);

	vec3 WorldPos = gBufferData.Position;

	// reflectance equation
	vec3 Lo = vec3(0.0);
	//for (int i = 0; i < 4; ++i)
	for (const auto& light : cGlobalBuffer.Lights)
	{
		vec3 lightPos = light.Position;
		vec3 lightColor	  = light.Color * 2.0f;
		// calculate per-light radiance
		vec3  L			  = normalize(lightPos - WorldPos);
		vec3  H			  = normalize(V + L);
		float distance	  = length(lightPos - WorldPos);
		float attenuation = 1.0 / (distance * distance);
		vec3  radiance	  = lightColor * attenuation;

		// Cook-Torrance BRDF
		float NDF = D_GGX(N, H, roughness);
		float G	  = GeometrySmith(N, V, L, roughness);
		vec3  F	  = F_Schlick(clamp(dot(H, V), 0.0f, 1.0f), F0);

		vec3  numerator	  = NDF * G * F;
		float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.0001f; // + 0.0001 to prevent divide by zero
		vec3  specular	  = numerator / denominator;

		// kS is equal to Fresnel
		vec3 kS = F;
		// for energy conservation, the diffuse and specular light can't
		// be above 1.0 (unless the surface emits light); to preserve this
		// relationship the diffuse component (kD) should equal 1.0 - kS.
		vec3 kD = vec3(1.0) - kS;
		// multiply kD by the inverse metalness such that only non-metals
		// have diffuse lighting, or a linear blend if partly metal (pure metals
		// have no diffuse light).
		kD *= 1.0 - metallic;

		// scale light by NdotL
		float NdotL = max(dot(N, L), 0.0f);

		// add to outgoing radiance Lo
		Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
	} 

	// ambient lighting (we now use IBL as the ambient term)
	vec3 F = F_SchlickR(max(dot(N, V), 0.0f), F0, roughness);

	vec3 kS = F;
	vec3 kD = 1.0f - kS;
	kD *= 1.0f - metallic;

	//vec3 irradiance = textureCube(EnvTexture, N).rgb;
	vec2 nUV		 = SampleSphericalMap(glm::normalize(N));
	vec3 irradiance = textureCubeLod(gEnvironmentData.IrradianceTexture, WorldPos, 0).rgb;
	vec3 diffuse	= irradiance * albedo;

	//// sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
	const float MAX_REFLECTION_LOD = 4.0;
	vec2		rUV				   = SampleSphericalMap(glm::normalize(R));
	vec3		prefilteredColor   = textureCubeLod(gEnvironmentData.EnvTexture, glm::normalize(R), int(roughness * MAX_REFLECTION_LOD)).rgb;
	//vec3		prefilteredColor   = texture2D(gEnvironmentData.SphericalEnvTexture, rUV, int(roughness * MAX_REFLECTION_LOD)).rgb;
	vec2 brdf			  = texture2D(gEnvironmentData.BRDFTexture, vec2(max(dot(N, V), 0.0f), roughness)).rg;
	vec3		specular		   = prefilteredColor * (F * brdf.x + brdf.y);

	vec3 ambient = (kD * diffuse + specular + gBufferData.EmissiveColor) * ao;

	vec3 color = ambient + Lo ;

	// HDR tonemapping
	color = color / (color + vec3(1.0));
	// gamma correct
	color = pow(color, vec3(1.0 / 2.2));

	FragColor = vec4(color, 1.0);
	return FragColor;

}