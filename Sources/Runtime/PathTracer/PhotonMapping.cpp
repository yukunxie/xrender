
#include "PhotonMapping.h"
#include <span>
#include <random>
#include <glm/gtc/random.hpp>

struct Photon
{
	vec3  HitPos;
	vec3  LightPos;
	vec3  LightShapeColor;
	vec4  flux;	 // RGBE
	float Phi;	 //入射光方向
	float Theta; //入射光方向
	short Flag;	 //用于KD-Tree结构的数据
};

std::vector<Photon> PhotonPositions;

static glm::vec3 GetDirectionFromAlphaBeta(float alpha, float beta)
{
	glm::vec3 direction;
	direction.x = cos(beta) * sin(alpha);
	direction.y = sin(beta);
	direction.z = cos(beta) * cos(alpha);
	return glm::normalize(direction);
}

static vec3 GenerateRandomDirection()
{
	static unsigned								 seed = 15375;
	static std::mt19937							 generator(seed);
	static std::uniform_real_distribution<float> uniform01(0.0f, 1.0f);

	// incorrect way
	float theta = 2 * M_PI * uniform01(generator);
	float phi	= M_PI * uniform01(generator);
	float x		= sin(phi) * cos(theta);
	float y		= sin(phi) * sin(theta);
	float z		= cos(phi);
	return glm::normalize(glm::vec3(x, y, z));
}

static bool RussianRoulette(float P)
{
	static std::random_device				rd;
	static std::mt19937						gen(rd());
	static std::uniform_real_distribution<> dis(0, 1);

	return dis(gen) < P;
}

static float GenerateRandom()
{
	static std::random_device				rd;
	static std::mt19937						gen(rd());
	static std::uniform_real_distribution<> dis(0, 1);

	return dis(gen);
}

PhotonMapper::PhotonMapper(const RTContext& context)
	: Raytracer(context)
{
}

static void DebugPhoton(const Photon& p, const RTContext& mContext)
{
	auto ViewMatrix = mContext.GlobalParameters.ViewMatrix;
	auto ProjMatrix = mContext.GlobalParameters.ProjMatrix;

	auto mvp = ProjMatrix * ViewMatrix;

	int w = mContext.RenderTargetColor->GetWidth();
	int h = mContext.RenderTargetColor->GetHeight();

	vec4 np = mvp * vec4(p.HitPos, 1.0f);
	np /= np.w;

	np.xyz = np.xyz * 0.5f + 0.5f;

	np.y = 1 - np.y;

	if (np.x >= 0 && np.x < 1 && np.y >= 0 && np.y < 1 && np.z >= 0 && np.z < 1)
	{
		int	  px	= int(np.x * w);
		int	  py	= int(np.y * h);
		float depth = mContext.RenderTargetDepth->ReadPixel(px, py).r;
		//if (depth > np.z)
		{
			mContext.RenderTargetColor->WritePixel(int(np.x * w), int(np.y * h), Color4f(p.LightShapeColor, 1.0f));
		}
	}
}

static void DebugLight(const LightData& light, const RTContext& mContext)
{
	constexpr int MAX_COUNT = 100;
	const float	  delta		= 1.0f / MAX_COUNT;
	for (int i = 0; i < MAX_COUNT; i++)
	{
		for (int j = 0; j < MAX_COUNT; j++)
		{
			for (int t = 0; t < MAX_COUNT; t++)
			{
				float x = (i - MAX_COUNT / 2) * delta;
				float y = (j - MAX_COUNT / 2) * delta;
				float z = (t - MAX_COUNT / 2) * delta;

				Photon photon;
				photon.HitPos		   = light.Position + vec3(x, y, z);
				photon.LightShapeColor = light.ShapeColor;

				if (glm::length(photon.HitPos - light.Position) < 0.5)
				{
					DebugPhoton(photon, mContext);
				}
			}
		}
	}
}

void PhotonMapper::RenderAsync() noexcept
{
	constexpr uint32 MAX_PHOTON_PER_LIGHT = 200000;
	const float		 LIGHT_FLUX			  = 15 * 100000.f;

	for (auto& light : mContext.GlobalParameters.Lights)
	{

#if 1
		DebugLight(light, mContext);
#endif

		for (int i = 0; i < MAX_PHOTON_PER_LIGHT; ++i)
		{
			vec3  dir = GenerateRandomDirection();
			RxRay ray;
			ray.Mask		= 0xFFFFFFFF;
			ray.MaxDistance = 5;
			ray.Origion		= light.Position;
			ray.Direction	= dir;
			vec4 flux		= vec4(light.Color, (LIGHT_FLUX / MAX_PHOTON_PER_LIGHT));
			EmitPhoton(0, ray, flux, light);
		}
	}

	if (true)
	{
	}
}

vec3 SampleHemisphere(const vec3& normal)
{
	glm::vec3 sample;
	do
	{
		float r1	= rand() / (float)RAND_MAX;
		float r2	= rand() / (float)RAND_MAX;
		float theta = 2 * M_PI * r1;
		float phi	= acos(2 * r2 - 1);
		float x		= sin(phi) * cos(theta);
		float y		= sin(phi) * sin(theta);
		float z		= cos(phi);
		sample		= glm::vec3(x, y, z);

	} while (glm::dot(sample, normal) <= 0);

	return glm::normalize(sample);
}

glm::vec3 CalculateReflectionDirection(const glm::vec3& photonDir, const glm::vec3 normal, vec3 tangent, vec3 bitangent, float roughness, bool metallic)
{
	glm::vec3 reflectDir = glm::reflect(photonDir, normal); // 首先计算镜面反射方向

	return reflectDir;

	glm::vec3 randDir = SampleHemisphere(normal);

	float roughnessFactor = roughness * roughness;
	vec3  newReflectDir	  = glm::normalize(reflectDir + randDir * roughnessFactor);

	// if (!metallic)
	//{
	//	glm::vec3 microNormal = glm::normalize(glm::vec3(glm::pow(glm::tan(glm::acos(roughness)), 2.0f), 0.0f, 1.0f)); // 计算微平面法线
	//	glm::mat3 TBN		  = glm::mat3(tangent, bitangent, normal);											   // 构建TBN矩阵
	//	microNormal			  = TBN * microNormal;																	   // 将微平面法线旋转到切面坐标系下
	//	// 根据微平面法线的分布情况计算出反射光线的方向
	//	glm::vec3 randDir = SampleHemisphere(normal);
	//	glm::vec3 newDir  = glm::normalize(glm::reflect(-photonDir, microNormal) + randDir * roughness); // 根据GGX分布计算新方向
	//	reflectDir		  = TBN * newDir;

	//	if (glm::any(glm::isnan(reflectDir)))
	//	{
	//		//Assert(false);
	//	}
	//}
	return newReflectDir;
}

float Pow5(float v)
{
	const float a = v * v;
	return a * a * v;
}

////法线分布函数
//inline float GGXTerm(float NdotH, float roughness)
//{
//	float a2 = roughness * roughness;
//	float d	 = (NdotH * a2 - NdotH) * NdotH + 1.0f;
//	return INV_PI * a2 / (d * d + 1e-7f);
//}
//
////正态分布函数
//float Distribution(float roughness, float nh)
//{
//	float lerpSquareRoughness = pow(glm::lerp(0.002f, 1.0f, roughness), 2);
//	float D					  = lerpSquareRoughness / (pow((pow(nh, 2) * (lerpSquareRoughness - 1) + 1), 2) * PI);
//	return D;
//}
//
////近似的菲涅尔函数
//vec3 FresnelSchlick(vec3 F0, float VdotH)
//{
//	vec3 F = F0 + (vec3(1) - F0) * exp2((-5.55473f * VdotH - 6.98316f) * VdotH);
//	return F;
//}
//
//// Cook-Torrance BRDF
//vec3 CookTorranceBRDF(float NdotH, float NdotL, float NdotV, float VdotH, float roughness, vec3 F0)
//{
//	float D	  = GGXTerm(NdotH, roughness);				//法线分布函数
//	float G	  = GeometrySmith(NdotV, NdotL, roughness); //微平面间相互遮蔽的比率
//	vec3  F	  = FresnelSchlick(F0, VdotH);				//近似的菲涅尔函数
//	vec3  res = (D * G * F * 0.25f) / (NdotV * NdotL);
//	return res;
//}
//
//// Note: Disney diffuse must be multiply by diffuseAlbedo / PI. This is done outside of this function.
//float DisneyDiffuse(float NdotV, float NdotL, float LdotH, float perceptualRoughness)
//{
//	float fd90 = 0.5 + 2 * LdotH * LdotH * perceptualRoughness;
//	// Two schlick fresnel term
//	float lightScatter = (1 + (fd90 - 1) * Pow5(1 - NdotL));
//	float viewScatter  = (1 + (fd90 - 1) * Pow5(1 - NdotV));
//	return lightScatter * viewScatter;
//}

// -------------------------------------------------------------------------
// Creating the BRDF (Bidirectional Reflective Distribution Function)
// In this section we create our three functions that are used in the Cook-Torrance Reflectance Equation
// These are: The Normal Distribution Function, The Geometry Function and The Fresnel Equation.
// -------------------------------------------------------------------------

// The Normal Distribution Function
float DistributionGGX(vec3 N, vec3 H, float a)
{
	// N, normal of the surface
	// H, halfway vector
	// a, surface roughness

	// We are doing our approximations based on the Trowbridge-Reits GGX:
	float a2	 = a * a;
	float NdotH	 = max(dot(N, H), 0.0f);
	float NdotH2 = NdotH * NdotH;

	float nom	= a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom		= PI * denom * denom;

	// The normal distribution function returns a value indicating how much o the surface's
	// microfacets are aligned to the incoming halfway vector.
	return nom / denom;
}

vec3 random_in_unit_sphere()
{
	// Returns a random direction in unit sphere (used in the BRDF)
	float phi	   = 2.0 * PI * GenerateRandom();
	float cosTheta = 2.0 * GenerateRandom() - 1.0;
	float u		   = GenerateRandom();

	float theta = acos(cosTheta);
	float r		= pow(u, 1.0 / 3.0);

	// Change of variables
	// Spherical Coordinates -> Carthesian Coordinates, to get (x, y, z) values
	float x = r * sin(theta) * cos(phi);
	float y = r * sin(theta) * sin(phi);
	float z = r * cos(theta);

	return vec3(x, y, z);
}

vec3 getReflection(vec3 Li, vec3 Normal, float rougness)
{
	/*
	Calculating the reflection direction based on the specular attribute of the material.
	Either it is a perfect reflection (specular = 1), a perfect diffuse (specular = 0) or
	something in between.
	*/
	vec3 diffuseDir	 = normalize(Normal + random_in_unit_sphere());
	vec3 specularDir = reflect(Li, Normal);

	vec3 nextDir = mix(diffuseDir, specularDir, rougness);
	return normalize(nextDir);
}


void _CalcDiffuseSpecularBRDF(vec3 albedo, vec3 N, vec3 Lo, vec3 Li, float metallic, float roughness, vec3& brdfDiffuse, vec3& brdfSpecular)
{
	vec3 L = -Li;
	vec3 V = Lo;
	vec3 H = glm::normalize(V + L);

	float a = glm::dot(N, H);
	float b = glm::dot(N, L);
	float c = glm::dot(N, V);

	// N = glm::abs(N);
	//  Cook-Torrance BRDF
	float D	 = D_GGX(N, H, roughness);
	float G	 = GeometrySmith(N, V, L, roughness);
	vec3  F0 = mix(vec3(0.04), albedo, metallic);
	vec3  F	 = fresnelSchlick(clamp(dot(H, V), 0.0f, 1.0f), F0);
	//vec3 F = F_SchlickR(clamp(dot(N, V), 0.0f, 1.0f), F0, roughness);

	vec3  numerator	  = D * G * F;
	float NdotV		  = dot(N, V);
	float NdotL		  = dot(N, L);
	float denominator = 4.0f * max(NdotV, 0.0f) * max(NdotL, 0.0f) + 0.0001f; // + 0.0001 to prevent divide by zero
	vec3  specular	  = numerator / denominator;

	//vec3  F0	   = mix(vec3(0.04), albedo, metallic);
	//float NdotV	   = dot(N, V);
	//float NdotL	   = dot(N, L);
	//float NdotH	   = dot(N, H);
	//float VdotH	   = dot(V, H);
	//vec3  specular = CookTorranceBRDF(NdotH, NdotL, NdotV, VdotH, roughness, F0);

	//vec3 F = FresnelSchlick(F0, VdotH);		

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
	brdfDiffuse	 = kD * albedo / PI * NdotL;
	brdfSpecular = specular * NdotL;


	//Assert(glm::all(glm::lessThanEqual(brdfDiffuse + brdfSpecular, vec3(1.0f))));
}

void PhotonMapper::EmitPhoton(int depth, const RxRay& ray, vec4 flux, const LightData& light) noexcept
{
	if (depth > 10)
		return;

	RxIntersection intersection = Intersection(ray);

	if (!intersection.IsHit)
	{
		return;
	}
	VertexOutputData vertexData;
	vec3			 albedo;
	float			 roughness;
	float			 metallic;

	intersection.SampleAttributes(mContext, vertexData, albedo, roughness, metallic);

	vec3 N = glm::normalize(vertexData.Normal);
	vec3 L = glm::normalize(ray.Direction);

	// inner space
	if (glm::dot(N, -L) < 0)
	{
		return;
	}

	vec3 reflectDirection = getReflection(L, N, roughness);
	/*do
	{
		reflectDirection = CalculateReflectionDirection(ray.Direction, vertexData.Normal, vertexData.Tangent, vertexData.BiTangent, roughness, false);
	} while (glm::any(glm::isnan(reflectDirection)));*/

	vec3 V = glm::normalize(reflectDirection);

	float a0 = glm::dot(N, V);
	float a	 = acosf(glm::dot(N, V));
	float b0 = glm::dot(N, -L);
	float b	 = acosf(glm::dot(N, -L));

	Assert(glm::dot(N, V) > 0);

	// void _CalcDiffuseSpecularBRDF(vec3 albedo, vec3 N, vec3 Lo, vec3 Li, float metallic, float roughness, vec3& brdfDiffuse, vec3& brdfSpecular)

	vec3 brdfDiffuse;
	vec3 brdfSpecular;
	Assert(roughness <= 1.0f && roughness >= 0.0f);
	_CalcDiffuseSpecularBRDF(albedo, N, V, L, metallic, roughness, brdfDiffuse, brdfSpecular);

	float maxDiffuse  = glm::max(glm::max(brdfDiffuse.x, brdfDiffuse.y), brdfDiffuse.z);
	float maxSepcular = glm::max(glm::max(brdfSpecular.x, brdfSpecular.y), brdfSpecular.z);


	float p = GenerateRandom();

	if (p < maxDiffuse)
	{
		Photon photon;
		photon.LightShapeColor = light.ShapeColor;
		photon.HitPos		   = vertexData.Position;
		photon.LightPos		   = ray.Origion;
		if (depth != 0)
		{
			PhotonPositions.push_back(photon);
#if 1
			DebugPhoton(photon, mContext);
#endif
		}
	}
	else if (p < (maxDiffuse + maxSepcular))
	{
		RxRay newRay	 = ray;
		newRay.Direction = reflectDirection;
		newRay.Origion	 = vertexData.Position + reflectDirection * 0.00001f;

		EmitPhoton(depth + 1, newRay, flux, light);
	}
	else
	{
		printf("xxxxxxxxxxxx\n");
		// absorbed
	}
}