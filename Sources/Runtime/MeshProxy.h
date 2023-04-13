#pragma once

#include <vector>

#include "Types.h"

class VertexBuffer;
class Material;
class MeshComponent;

class AbstractMesh
{
};

class Mesh : public AbstractMesh
{
public:
	Mesh() = default;
};

class MeshBuilder
{
	// public:
	//	static Mesh* CreateBox();
	//
	//	static Mesh* CreateSphere();
};

//struct RTCMeshProxy
//{
//	RTCMeshProxy()
//	{
//	}
//
//	RTCMeshProxy(Geometry* geo, )
//
//	/*RTCMeshProxy(const std::vector<Vector2f>&	  uv,
//				 const std::vector<Vector3f>&	  normals,
//				 const std::vector<unsigned int>& indices)
//	{
//		if (!uv.empty())
//		{
//			UVs = (Vector2f*)malloc(uv.size() * sizeof(uv[0]));
//			if (UVs)
//			{
//				memcpy(UVs, uv.data(), uv.size() * sizeof(uv[0]));
//			}
//		}
//
//		if (!normals.empty())
//		{
//			Normals = (Vector3f*)malloc(normals.size() * sizeof(normals[0]));
//			if (Normals)
//			{
//				memcpy(Normals, normals.data(), normals.size() * sizeof(normals[0]));
//			}
//		}
//
//		Triangles = (Triangle*)malloc(indices.size() * sizeof(indices[0]));
//		if (Triangles)
//		{
//			memcpy(Triangles, indices.data(), indices.size() * sizeof(indices[0]));
//		}
//	}
//
//	~RTCMeshProxy()
//	{
//		if (UVs)
//			free(UVs);
//		if (Normals)
//			free(Normals);
//		if (Triangles)
//			free(Triangles);
//	}
//
//	Vector2f* UVs		= nullptr;
//	Vector3f* Normals	= nullptr;
//	Triangle* Triangles = nullptr;*/
//
//	VertexBuffer* mVertexBuffer = nullptr;
//	Material* mMaterial			= nullptr;
//};

void GenerateSphereSmooth(int						 radius,
						  int						 latitudes,
						  int						 longitudes,
						  std::vector<Vector3f>&	 vertices,
						  std::vector<Vector3f>&	 normals,
						  std::vector<Vector2f>&	 uv,
						  std::vector<unsigned int>& indices);

void GenerateCube(
std::vector<Vector3f>&	   vertices,
std::vector<Vector3f>&	   normals,
std::vector<Vector2f>&	   uv,
std::vector<unsigned int>& indices,
Vector3f				   scale = Vector3f(1.0f));