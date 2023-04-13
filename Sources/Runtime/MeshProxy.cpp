
#include "MeshProxy.h"
#include <span>

void GenerateSphereSmooth(int						 radius,
						  int						 latitudes,
						  int						 longitudes,
						  std::vector<Vector3f>&	 vertices,
						  std::vector<Vector3f>&	 normals,
						  std::vector<Vector2f>&	 uv,
						  std::vector<unsigned int>& indices)
{
	if (longitudes < 3)
		longitudes = 3;
	if (latitudes < 2)
		latitudes = 2;

	vertices.clear();
	normals.clear();
	uv.clear();
	indices.clear();

	float nx, ny, nz, lengthInv = 1.0f / radius; // normal

	// Temporary vertex
	struct Vertex
	{
		float x, y, z, s, t; // Postion and Texcoords
	};

	float deltaLatitude	 = M_PI / latitudes;
	float deltaLongitude = 2 * M_PI / longitudes;
	float latitudeAngle;
	float longitudeAngle;

	// Compute all vertices first except normals
	for (int i = 0; i <= latitudes; ++i)
	{
		latitudeAngle = M_PI / 2 - i * deltaLatitude; /* Starting -pi/2 to pi/2 */
		float xy	  = radius * cosf(latitudeAngle); /* r * cos(phi) */
		float z		  = radius * sinf(latitudeAngle); /* r * sin(phi )*/

		/*
		 * We add (latitudes + 1) vertices per longitude because of equator,
		 * the North pole and South pole are not counted here, as they overlap.
		 * The first and last vertices have same position and normal, but
		 * different tex coords.
		 */
		for (int j = 0; j <= longitudes; ++j)
		{
			longitudeAngle = j * deltaLongitude;

			Vertex vertex;
			vertex.x = xy * cosf(longitudeAngle); /* x = r * cos(phi) * cos(theta)  */
			vertex.y = xy * sinf(longitudeAngle); /* y = r * cos(phi) * sin(theta) */
			vertex.z = z;						  /* z = r * sin(phi) */
			vertex.s = j * 1.0f / longitudes;	  /* s */
			vertex.t = i * 1.0f / latitudes;	  /* t */
			vertices.push_back(Vector3f(vertex.x, vertex.y, vertex.z));
			uv.push_back(Vector2f(vertex.s, vertex.t));

			// normalized vertex normal
			nx = vertex.x * lengthInv;
			ny = vertex.y * lengthInv;
			nz = vertex.z * lengthInv;
			normals.push_back(Vector3f(nx, ny, nz));
		}
	}

	/*
	 *  Indices
	 *  k1--k1+1
	 *  |  / |
	 *  | /  |
	 *  k2--k2+1
	 */
	unsigned int k1, k2;
	for (int i = 0; i < latitudes; ++i)
	{
		k1 = i * (longitudes + 1);
		k2 = k1 + longitudes + 1;
		// 2 Triangles per latitude block excluding the first and last longitudes blocks
		for (int j = 0; j < longitudes; ++j, ++k1, ++k2)
		{
			if (i != 0)
			{
				indices.push_back(k1);
				indices.push_back(k2);
				indices.push_back(k1 + 1);
			}

			if (i != (latitudes - 1))
			{
				indices.push_back(k1 + 1);
				indices.push_back(k2);
				indices.push_back(k2 + 1);
			}
		}
	}
}


void GenerateCube(std::vector<Vector3f>&	 vertices,
				  std::vector<Vector3f>&	 normals,
				  std::vector<Vector2f>&	 uv,
				  std::vector<unsigned int>& indices,
				  Vector3f					 scale)
{
	vertices.resize(8);
	vertices[0] = Vector3f{ -1, -1, -1 } * scale;
	vertices[1] = Vector3f{ -1, -1, +1 } * scale;
	vertices[2] = Vector3f{ -1, +1, -1 } * scale;
	vertices[3] = Vector3f{ -1, +1, +1 } * scale;
	vertices[4] = Vector3f{ +1, -1, -1 } * scale;
	vertices[5] = Vector3f{ +1, -1, +1 } * scale;
	vertices[6] = Vector3f{ +1, +1, -1 } * scale;
	vertices[7] = Vector3f{ +1, +1, +1 } * scale;

	uv.resize(8);

	uv[0] = {0, 0};
	uv[1] = {0, 1};
	uv[2] = {1, 0};
	uv[3] = {1, 1};
	uv[4] = {0, 0};
	uv[5] = {0, 1};
	uv[6] = {1, 0};
	uv[7] = {1, 1};

	indices.resize(6 * 2 * 3);

	std::span<Triangle> triangles((Triangle*)indices.data(), indices.size() / 3);

	int tri = 0;
	// left side
	triangles[tri] = {0, 2, 1};
	tri++;
	triangles[tri] = {1, 2, 3};
	tri++;
	// right side
	triangles[tri] = {4, 5, 6};
	tri++;
	triangles[tri] = {5, 7, 6};
	tri++;
	// bottom side
	triangles[tri] = { 0, 1, 4 };
	tri++;
	triangles[tri] = {1, 5, 4};
	tri++;
	// top side
	triangles[tri] = { 2, 6, 3 };
	tri++;
	triangles[tri] = { 3, 6, 7 };
	tri++;
	// front side
	triangles[tri] = { 0, 4, 2 };
	tri++;
	triangles[tri] = { 2, 4, 6 };
	tri++;
	// back side
	triangles[tri] = { 1, 3, 5 };
	tri++;
	triangles[tri] = { 3, 7, 5 };
}