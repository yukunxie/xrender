
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
	short Flag;		  //用于KD-Tree结构的数据
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

PhotonMapper::PhotonMapper(const RTContext& context)
	: Raytracer(context)
{

}

static void DebugPhoton(const Photon& p, const RTContext& mContext)
{
	auto ViewMatrix = mContext.GlobalParameters.ViewMatrix;
	auto ProjMatrix = mContext.GlobalParameters.ProjMatrix;

	auto mvp = ProjMatrix * ViewMatrix;

	int	 w	= mContext.RenderTargetColor->GetWidth();
	int	 h	= mContext.RenderTargetColor->GetHeight();

	vec4 np = mvp * vec4(p.HitPos, 1.0f);
	np /= np.w;

	np.xyz = np.xyz * 0.5f + 0.5f;

	np.y = 1 - np.y;

	if (np.x >= 0 && np.x < 1 && np.y >= 0 && np.y < 1 && np.z >= 0 && np.z < 1)
	{
		int px = int(np.x * w);
		int py = int(np.y * h);
		float depth = mContext.RenderTargetDepth->ReadPixel(px, py).r;
		if (depth > np.z)
		{
			mContext.RenderTargetColor->WritePixel(int(np.x * w), int(np.y * h), Color4f(p.LightShapeColor, 1.0f));
		}
	}
}

static void DebugLight(const LightData& light, const RTContext &mContext)
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

	for (auto & light : mContext.GlobalParameters.Lights)
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
			vec4 flux = vec4(light.Color, (LIGHT_FLUX / MAX_PHOTON_PER_LIGHT)); 
			EmitPhoton(ray, flux, light);
		}
	}
}

vec3 SampleHemisphere(const vec3& normal)
{
	float	  r1	= rand() / (float)RAND_MAX;
	float	  r2	= rand() / (float)RAND_MAX;
	float	  theta = 2 * M_PI * r1;
	float	  phi	= acos(2 * r2 - 1);
	float	  x		= sin(phi) * cos(theta);
	float	  y		= sin(phi) * sin(theta);
	float	  z		= cos(phi);
	glm::vec3 sample(x, y, z);
	if (glm::dot(sample, normal) < 0)
	{
		sample *= -1;
	}
	return glm::normalize(sample);
}

glm::vec3 CalculateReflectionDirection(const glm::vec3& photonDir, const glm::vec3 normal, vec3 tangent, vec3 bitangent, float roughness, bool metallic)
{
	glm::vec3 reflectDir = glm::reflect(-photonDir, normal); // 首先计算镜面反射方向
	if (!metallic)
	{																												  
		glm::vec3 microNormal = glm::normalize(glm::vec3(glm::pow(glm::tan(glm::acos(roughness)), 2.0f), 0.0f, 1.0f)); // 计算微平面法线	
		glm::mat3 TBN		  = glm::mat3(tangent, bitangent, normal);											   // 构建TBN矩阵
		microNormal			  = TBN * microNormal;																	   // 将微平面法线旋转到切面坐标系下
		// 根据微平面法线的分布情况计算出反射光线的方向
		glm::vec3 randDir = SampleHemisphere(normal);
		glm::vec3 newDir  = glm::normalize(glm::reflect(-photonDir, microNormal) + randDir * roughness); // 根据GGX分布计算新方向
		reflectDir		  = TBN * newDir;	

		if (glm::any(glm::isnan(reflectDir)))
		{
			//Assert(false);
		}
	}
	return reflectDir;
}

void PhotonMapper::EmitPhoton(const RxRay& ray, vec4 flux, const LightData& light) noexcept
{
	RxIntersection intersection = Intersection(ray);

	if (!intersection.IsHit)
	{
		return;
	}
	VertexOutputData vertexData;
	float			 roughness;
	float			 metallic;

	intersection.SampleAttributes(mContext, vertexData, roughness, metallic);

	//vec3 newRay = CalculateReflectionDirection(ray.Direction, vertexData.Normal, roughness, false);

	if (RussianRoulette(0.5f))
	{
		/*if (auto l = glm::length(vertexData.Position - light.Position); l > 50.f)
		{
			std::cout << "(" << vertexData.Position.x << "," << vertexData.Position.y << "," << vertexData.Position.z << ") "
					  << "(" << light.Position.x << "," << light.Position.y << "," << light.Position.z << ") " << std::endl;
		}*/
		Photon photon;
		photon.LightShapeColor = light.ShapeColor;
		photon.HitPos = vertexData.Position;
		photon.LightPos = ray.Origion;
		PhotonPositions.push_back(photon);

#if 1
		DebugPhoton(photon, mContext);
#endif
	}
	else
	{
		RxRay newRay;
		newRay.Direction   = CalculateReflectionDirection(ray.Direction, vertexData.Normal, vertexData.Tangent, vertexData.BiTangent, roughness, false);

		if (!glm::any(glm::isnan(newRay.Direction)))
		{
			newRay.Origion	   = vertexData.Position + 0.00001f;
			newRay.MaxDistance = 100;

			EmitPhoton(newRay, flux, light);
		}
	}
}