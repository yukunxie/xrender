#include "PBRShader.h"
#include "Graphics/RxSampler.h"

#include <algorithm>

#include "ShadingBase.h"

#define COOK_GGX

// phong (lambertian) diffuse term
float phong_diffuse()
{
	return (1.0f / PI);
}

// compute fresnel specular factor for given base specular and product
// product could be NdV or VdH depending on used technique
vec3 fresnel_factor(vec3 f0, float product)
{
	return mix(f0, vec3(1.0f), pow(1.01f - product, 5.0f));
}

// following functions are copies of UE4
// for computing cook-torrance specular lighting terms

float D_blinn(float roughness, float NdH)
{
	float m	 = roughness * roughness;
	float m2 = m * m;
	float n	 = 2.0f / m2 - 2.0f;
	return (n + 2.0f) / (2.0f * PI) * pow(NdH, n);
}

float D_beckmann(float roughness, float NdH)
{
	float m	   = roughness * roughness;
	float m2   = m * m;
	float NdH2 = NdH * NdH;
	return exp((NdH2 - 1.0f) / (m2 * NdH2)) / (PI * m2 * NdH2 * NdH2);
}

// simple phong specular calculation with normalization
vec3 phong_specular(vec3 V, vec3 L, vec3 N, vec3 specular, float roughness)
{
	vec3  R	   = reflect(-L, N);
	float spec = max(0.0f, dot(V, R));

	float k = 1.999f / (roughness * roughness);

	return min(1.0f, 3.0f * 0.0398f * k) * pow(spec, min(10000.0f, k)) * specular;
}

// simple blinn specular calculation with normalization
vec3 blinn_specular(float NdH, vec3 specular, float roughness)
{
	float k = 1.999f / (roughness * roughness);

	return min(1.0f, 3.0f * 0.0398f * k) * pow(NdH, min(10000.0f, k)) * specular;
}

// cook-torrance specular calculation
vec3 cooktorrance_specular(float NdL, float NdV, float NdH, vec3 specular, float roughness)
{
#ifdef COOK_BLINN
	float D = D_blinn(roughness, NdH);
#endif

#ifdef COOK_BECKMANN
	float D = D_beckmann(roughness, NdH);
#endif

#ifdef COOK_GGX
	float D = D_GGX2(roughness, NdH);
#endif

	float G = G_schlick(roughness, NdV, NdL);

	float rim = mix(1.0f - roughness /** material.w*/ * 0.9f, 1.0f, NdV);

	return (1.0f / rim) * specular * G * D;
}

// https://gist.github.com/galek/53557375251e1a942dfa
// A: light attenuation
Color4f PBRShading(float metallic, float roughness, const TMat3x3& normalMatrix, Vector3f N, Vector3f V, Vector3f L, Vector3f H, float A, Vector3f Albedo, const Texture2D* BRDFTexture, const TextureCube* EnvTexture)
{
	//// point light direction to point in view space
	//vec3 local_light_pos = (view_matrix * (/*world_matrix */ light_pos)).xyz;

	//// light attenuation
	//float A = 20.0 / dot(local_light_pos - v_pos, local_light_pos - v_pos);

	//// L, V, H vectors
	//vec3 L	= normalize(local_light_pos - v_pos);
	//vec3 V	= normalize(-v_pos);
	//vec3 H	= normalize(L + V);
	vec3 nn = normalize(N);

	/*vec3   nb  = normalize(v_binormal);
	mat3x3 tbn = mat3x3(nb, cross(nn, nb), nn);*/


	//vec2 texcoord = v_texcoord;


	// normal map
#if USE_NORMAL_MAP
	// tbn basis
	vec3 N = tbn * (texture2D(norm, texcoord).xyz * 2.0 - 1.0);
#else
	N			= nn;
#endif

	// albedo/specular base
#if USE_ALBEDO_MAP
	vec3 base = texture2D(tex, texcoord).xyz;
#else
	vec3  base		= Albedo;
#endif

//	// roughness
//#if USE_ROUGHNESS_MAP
//	float roughness = texture2D(spec, texcoord).y * material.y;
//#else
//	float roughness = material.y;
//#endif

	//// material params
	//float metallic = material.x;

	// mix between metal and non-metal material, for non-metal
	// constant base specular factor of 0.04 grey is used
	vec3 specular = mix(vec3(0.04), base, metallic);

	//// diffuse IBL term
	////    I know that my IBL cubemap has diffuse pre-integrated value in 10th MIP level
	////    actually level selection should be tweakable or from separate diffuse cubemap
	//mat3x3 tnrm	   = transpose(normal_matrix);
	//vec3   envdiff = textureCubeLod(envd, tnrm * N, 10).xyz;
	vec3 envdiff = vec3(textureCube(EnvTexture, N, 10));

	//// specular IBL term
	////    11 magic number is total MIP levels in cubemap, this is simplest way for picking
	////    MIP level from roughness value (but it's not correct, however it looks fine)
	vec3 refl	 = normalMatrix * reflect(V, N);
	/*vec3 envspec = textureCubeLod(
				   EnvTexture, refl, max(roughness * 11.0f, textureQueryLod(envd, refl).y))
				   .xyz;*/

	vec3 envspec = vec3(textureCubeLod(EnvTexture, refl, 0));

	////vec3 envspec = vec3(0.3f);

	// compute material reflectance

	float NdL = max(0.0f, dot(N, L));
	float NdV = max(0.001f, dot(N, V));
	float NdH = max(0.001f, dot(N, H));
	float HdV = max(0.001f, dot(H, V));
	float LdV = max(0.001f, dot(L, V));

	// fresnel term is common for any, except phong
	// so it will be calcuated inside ifdefs


#ifdef PHONG
	// specular reflectance with PHONG
	vec3 specfresnel = fresnel_factor(specular, NdV);
	vec3 specref	 = phong_specular(V, L, N, specfresnel, roughness);
#endif

#ifdef BLINN
	// specular reflectance with BLINN
	vec3 specfresnel = fresnel_factor(specular, HdV);
	vec3 specref	 = blinn_specular(NdH, specfresnel, roughness);
#endif

#ifdef COOK_GGX
	// specular reflectance with COOK-TORRANCE
	vec3 specfresnel = fresnel_factor(specular, HdV);
	vec3 specref	 = cooktorrance_specular(NdL, NdV, NdH, specfresnel, roughness);
#endif

	specref *= vec3(NdL);

	// diffuse is common for any model
	vec3 diffref = (vec3(1.0) - specfresnel) * phong_diffuse() * NdL;


	// compute lighting
	vec3 reflected_light = vec3(0);
	vec3 diffuse_light	 = vec3(0); // initial value == constant ambient light

	// point light
	vec3 light_color = vec3(1.0) * A;
	reflected_light += specref * light_color;
	diffuse_light += diffref * light_color;

	// IBL lighting
	vec2 brdf	 = vec2(texture2D(BRDFTexture, vec2(roughness, 1.0 - NdV)));
	vec3 iblspec = min(vec3(0.99), fresnel_factor(specular, NdV) * brdf.x + brdf.y);
	reflected_light += iblspec * envspec;
	diffuse_light += envdiff * (1.0f / PI);

	// final result
	vec3 result =
	diffuse_light * mix(base, vec3(0.0), metallic) +
	reflected_light;

#if 0
	return vec4(0.5f * (nn + 1.0f), 1);
#else
	return vec4(envdiff, 1);
#endif
	
}

static glm::vec3 lightPositions[] = {
	glm::vec3(-10.0f, 10.0f, 10.0f),
	glm::vec3(10.0f, 10.0f, 10.0f),
	glm::vec3(-10.0f, -10.0f, 10.0f),
	glm::vec3(10.0f, -10.0f, 10.0f),
};
static glm::vec3 lightColors[] = {
	glm::vec3(300.0f, 300.0f, 300.0f),
	glm::vec3(300.0f, 300.0f, 300.0f),
	glm::vec3(300.0f, 300.0f, 300.0f),
	glm::vec3(300.0f, 300.0f, 300.0f)
};


// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a		 = roughness * roughness;
	float a2	 = a * a;
	float NdotH	 = max(dot(N, H), 0.0f);
	float NdotH2 = NdotH * NdotH;

	float nom	= a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0f);
	denom		= PI * denom * denom;

	return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0f);
	float k = (r * r) / 8.0f;

	float nom	= NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0f);
	float NdotL = max(dot(N, L), 0.0f);
	float ggx2	= GeometrySchlickGGX(NdotV, roughness);
	float ggx1	= GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0f - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
} 


static vec2 _SampleSphericalMap(vec3 v)
{
	static const vec2 invAtan = vec2(0.1591f, 0.3183f);
	vec2			  uv	  = vec2(glm::atan(v.z, v.x), asin(v.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

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

			irradiance += texture2D(environmentMap, _SampleSphericalMap(sampleVec)).rgb * cos(theta) * sin(theta);
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
	vec3  ao		= vec3(1.0f);
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
		float NDF = DistributionGGX(N, H, roughness);
		float G	  = GeometrySmith(N, V, L, roughness);
		vec3  F	  = fresnelSchlick(clamp(dot(H, V), 0.0f, 1.0f), F0);

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
	vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0f), F0, roughness);

	vec3 kS = F;
	vec3 kD = 1.0f - kS;
	kD *= 1.0f - metallic;

	//vec3 irradiance = textureCube(EnvTexture, N).rgb;
	vec2 nUV		 = _SampleSphericalMap(glm::normalize(N));
	vec3 irradiance = texture2D(gEnvironmentData.SphericalEnvTexture, vec2(nUV.x, nUV.y), 0).rgb;
	//vec3 irradiance = CalcIrradiance(gEnvironmentData.SphericalEnvTexture, WorldPos).rgb;
	vec3 diffuse	= irradiance * albedo;

	//// sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
	const float MAX_REFLECTION_LOD = 4.0;
	vec2		rUV				   = _SampleSphericalMap(glm::normalize(R));
	//vec3		prefilteredColor   = textureCubeLod(EnvTexture, R, int(roughness * MAX_REFLECTION_LOD)).rgb;
	vec3 prefilteredColor = texture2D(gEnvironmentData.SphericalEnvTexture, R, int(roughness * MAX_REFLECTION_LOD)).rgb;
	vec2 brdf			  = texture2D(gEnvironmentData.BRDFTexture, vec2(max(dot(N, V), 0.0f), 1-roughness)).rg;
	vec3		specular		   = prefilteredColor * (F * brdf.x + brdf.y);

	vec3 ambient = (kD * diffuse + specular) * ao;

	vec3 color = ambient + Lo;

	// HDR tonemapping
	color = color / (color + vec3(1.0));
	// gamma correct
	color = pow(color, vec3(1.0 / 2.2));

	FragColor = vec4(prefilteredColor, 1.0);
	return FragColor;


	return vec4(specular, 1);
}