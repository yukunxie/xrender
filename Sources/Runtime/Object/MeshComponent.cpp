

#include "MeshComponent.h"
#include "Object/Entity.h"
#include "Meshes/Geometry.h"
#include "VertexBuffer.h"

#include <glm/ext/matrix_transform.hpp>

#include <math.h>

#ifndef PI
#	define PI 3.1415926535897932384626433832795f
#endif 

void BindVertexBufferHandle(MeshComponent* meshComp, VertexBufferAttriKind kind, InputAttributeFormat format, const void* data, uint32 size)
{
	auto vbBuffer	 = new VertexBuffer();
	vbBuffer->kind	 = kind;
	vbBuffer->format = format;
	vbBuffer->InitData(data, size);
	meshComp->GetGeometry()->AppendVertexBuffer(vbBuffer);
};

Vector3f RandomColor()
{
	float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX); // 生成0到1之间的随机数
	float g = static_cast<float>(rand()) / static_cast<float>(RAND_MAX); // 生成0到1之间的随机数
	float b = static_cast<float>(rand()) / static_cast<float>(RAND_MAX); // 生成0到1之间的随机数

	// 计算颜色的灰度值
	float gray = (r * 0.3) + (g * 0.59) + (b * 0.11);

	// 根据灰度值调整颜色的亮度和对比度
	r = (r - gray) * 3.0;
	g = (g - gray) * 3.0;
	b = (b - gray) * 3.0;

	// 确保颜色值在0到1之间
	r = glm::clamp(r, 0.0f, 1.0f);
	g = glm::clamp(g, 0.0f, 1.0f);
	b = glm::clamp(b, 0.0f, 1.0f);

	return Vector3f(r, g, b); // 返回随机颜色向量
}

MeshComponent::MeshComponent()
{
	MeshDebugColor_ = RandomColor();
}

MeshComponent::~MeshComponent()
{
	/*if (Engine::GetWorld()->CurrentSelectedMeshComponent == this)
	{
		Engine::GetWorld()->CurrentSelectedMeshComponent = nullptr;
	}*/
}

void MeshComponent::SetupRenderObject()
{
	////RenderObject_.bSelected = IsSelected();

	// if (RenderObject_.MaterialObject)
	//{
	//	return;
	// }
	// RenderObject_.RenderSetBits = RenderSetBits_;
	// RenderObject_.MaterialObject = Material_;
	// auto& ib = RenderObject_.IndexBuffer;
	//{
	//	ib.GpuBuffer = Geometry_->GetIndexBuffer()->gpuBuffer;
	//	ib.IndexCount = Geometry_->GetIndexBuffer()->GetIndexCount();
	//	ib.InstanceCount = 0;
	//	ib.IndexType = IndexType::UINT32;
	//	ib.Offset = 0;
	// }

	////InputAssembler

	// std::vector<InputAttribute> attributes;

	// RenderObject_.VertexBuffers.clear();

	// uint32 slot = 0;

	// for (auto& vbs : Geometry_->GetVBStreams())
	//{
	//	RenderObject::VertexBufferInfo vb;
	//	{
	//		vb.GpuBuffer = vbs->gpuBuffer;
	//		vb.Offset = 0;
	//		vb.Slot = slot++;
	//	}
	//	RenderObject_.VertexBuffers.push_back(vb);

	//	InputAttribute iAttri;
	//	{
	//		iAttri.format = vbs->format;
	//		iAttri.kind = vbs->kind;
	//		iAttri.location = vb.Slot;
	//		iAttri.stride = vbs->byteStride ? vbs->byteStride : GetFormatSize(iAttri.format);
	//	}
	//	attributes.push_back(iAttri);
	//}

	// Material_->SetInputAssembler({ attributes , IndexType::UINT32 });
}

void MeshComponent::SetSelected(bool selected)
{
	/*if (IsSelected_ == selected)
	{
		return;
	}

	IsSelected_ = selected;
	auto world = Engine::GetWorld();

	if (!IsSelected_ && world->CurrentSelectedMeshComponent == this)
	{
		world->CurrentSelectedMeshComponent = nullptr;
	}
	else if (IsSelected_ && world->CurrentSelectedMeshComponent != this)
	{
		if (world->CurrentSelectedMeshComponent)
		{
			world->CurrentSelectedMeshComponent->SetSelected(false);
		}
		world->CurrentSelectedMeshComponent = this;
	}*/
}

void MeshComponent::Tick(float dt)
{
	SetupRenderObject();

	const auto* etOwner = GetOwner();
	if (etOwner)
	{
		const TMat4x4& worldMatrix = etOwner->GetWorldMatrix();
		Material_->SetFloat("WorldMatrix", 0, 16, (float*)&worldMatrix);
	}
	else
	{
		const auto& matrix = TMat4x4(1.0f);
		;
		Material_->SetFloat("WorldMatrix", 0, 16, (float*)&matrix);
	}

	// Engine::GetRenderScene()->AddRenderObject(&RenderObject_);
}

MeshComponent* MeshComponentBuilder::CreateBox(const std::string& material, const Vector3f scale)
{
	MeshComponent* meshComp = new MeshComponent();
	meshComp->Geometry_		= new Geometry;
	meshComp->Material_		= new Material(material.empty() ? "Materials/PBR.json" : material);

	std::vector<TVector3> positions = {
		// Front face
		{ -1.0f, -1.0f, 1.0f },
		{ 1.0f, -1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ -1.0f, 1.0f, 1.0f },
		// Back face
		{ -1.0f, -1.0f, -1.0f },
		{ -1.0f, 1.0f, -1.0f },
		{ 1.0f, 1.0f, -1.0f },
		{ 1.0f, -1.0f, -1.0f },
		// Top face
		{ -1.0f, 1.0f, -1.0f },
		{ -1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, -1.0f },
		// Bottom face
		{ -1.0f, -1.0f, -1.0f },
		{ 1.0f, -1.0f, -1.0f },
		{ 1.0f, -1.0f, 1.0f },
		{ -1.0f, -1.0f, 1.0f },
		// Right face
		{ 1.0f, -1.0f, -1.0f },
		{ 1.0f, 1.0f, -1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, -1.0f, 1.0f },
		// Left face
		{ -1.0f, -1.0f, -1.0f },
		{ -1.0f, -1.0f, 1.0f },
		{ -1.0f, 1.0f, 1.0f },
		{ -1.0f, 1.0f, -1.0f },
	};

	for (auto& p : positions)
	{
		p *= scale;
	}

	std::vector<TVector3> colors = {
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },

		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },

		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },

		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },

		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },

		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
	};

	std::vector<TVector2> texCoords = {
		{ 1.0f, 0.0f },
		{ 0.0f, 0.0f },
		{ 0.0f, 1.0f },
		{ 1.0f, 1.0f },
		{ 1.0f, 0.0f },
		{ 0.0f, 0.0f },
		{ 0.0f, 1.0f },
		{ 1.0f, 1.0f },
		{ 1.0f, 0.0f },
		{ 0.0f, 0.0f },
		{ 0.0f, 1.0f },
		{ 1.0f, 1.0f },
		{ 1.0f, 0.0f },
		{ 0.0f, 0.0f },
		{ 0.0f, 1.0f },
		{ 1.0f, 1.0f },
		{ 1.0f, 0.0f },
		{ 0.0f, 0.0f },
		{ 0.0f, 1.0f },
		{ 1.0f, 1.0f },
		{ 1.0f, 0.0f },
		{ 0.0f, 0.0f },
		{ 0.0f, 1.0f },
		{ 1.0f, 1.0f }
	};


	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::POSITION, InputAttributeFormat::FLOAT3, positions.data(), sizeof(positions[0]) * positions.size());
	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::DIFFUSE, InputAttributeFormat::FLOAT3, colors.data(), sizeof(colors[0]) * colors.size());
	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::TEXCOORD, InputAttributeFormat::FLOAT2, texCoords.data(), sizeof(texCoords[0]) * texCoords.size());

	std::vector<std::uint32_t> indices = {
		// front
		0, 1, 2, 0, 2, 3,
		// back
		4, 5, 6, 4, 6, 7,
		// top
		8, 9, 10, 8, 10, 11, 
		// bottom
		12, 13, 14, 12, 14, 15, 
		// right
		16, 17, 18, 16, 18, 19, 
		// left
		20, 21, 22, 20, 22, 23,
	};

	std::vector<TVector3> normals(positions.size());

	for (uint32 face = 0; face < 6; ++face)
	{
		uint32			idx = face * 6;
		const TVector3& v0	= positions[indices[idx]];
		const TVector3& v1	= positions[indices[idx + 1]];
		const TVector3& v2	= positions[indices[idx + 2]];

		TVector3 d0 = v1 - v0;
		TVector3 d1 = v2 - v0;

		TVector3 normal = -glm::normalize(glm::cross(d1, d0));

		for (uint32 i = 0; i < 6; i++)
		{
			normals[indices[idx + i]] = normal;
		}
	}
	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::NORMAL, InputAttributeFormat::FLOAT3, normals.data(), sizeof(normals[0]) * normals.size());

	meshComp->Geometry_->indexBuffer_.indexType = IndexType::UINT32;
	meshComp->Geometry_->indexBuffer_.InitData(indices.data(), indices.size() * sizeof(indices[0]));


	return meshComp;
}

MeshComponent* MeshComponentBuilder::CreateSkyBox(const Vector3f scale)
{
	MeshComponent* meshComp = new MeshComponent();
	meshComp->Geometry_		= new Geometry;
	meshComp->Material_		= new Material("Materials/PBR.json");
	meshComp->Material_->SetRenderCore(std::make_shared<RenderCoreSkybox>());

	std::vector<TVector3> positions = {
		// Front face
		{ -1.0f, -1.0f, 1.0f },
		{ 1.0f, -1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ -1.0f, 1.0f, 1.0f },
		// Back face
		{ -1.0f, -1.0f, -1.0f },
		{ -1.0f, 1.0f, -1.0f },
		{ 1.0f, 1.0f, -1.0f },
		{ 1.0f, -1.0f, -1.0f },
		// Top face
		{ -1.0f, 1.0f, -1.0f },
		{ -1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, -1.0f },
		// Bottom face
		{ -1.0f, -1.0f, -1.0f },
		{ 1.0f, -1.0f, -1.0f },
		{ 1.0f, -1.0f, 1.0f },
		{ -1.0f, -1.0f, 1.0f },
		// Right face
		{ 1.0f, -1.0f, -1.0f },
		{ 1.0f, 1.0f, -1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, -1.0f, 1.0f },
		// Left face
		{ -1.0f, -1.0f, -1.0f },
		{ -1.0f, -1.0f, 1.0f },
		{ -1.0f, 1.0f, 1.0f },
		{ -1.0f, 1.0f, -1.0f },
	};

	for (auto& p : positions)
	{
		p *= scale;
	}

	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::POSITION, InputAttributeFormat::FLOAT3, positions.data(), sizeof(positions[0]) * positions.size());

	// CCW
	std::vector<std::uint32_t> indices = {
		// front
		0, 1, 2, 0, 2, 3,
		// back
		4, 5, 6, 4, 6, 7,
		// top
		8, 9, 10, 8, 10, 11, 
		// bottom
		12, 13, 14, 12, 14, 15, 
		// right
		16, 17, 18, 16, 18, 19, 
		// left
		20, 21, 22, 20, 22, 23,
	};

	//// Convert to CW, because skybox view in the box
	//for (int i = 0; i < indices.size(); i += 3)
	//{
	//	std::swap(indices[i], indices[i + 1]);
	//}

	meshComp->Geometry_->indexBuffer_.indexType = IndexType::UINT32;
	meshComp->Geometry_->indexBuffer_.InitData(indices.data(), indices.size() * sizeof(indices[0]));


	return meshComp;
}

static void _CalculateTangentBitangent(const std::span<vec3>&	  vertices,
									   const std::span<vec2>&	  uvs,
									   const std::span<uint32>& indices,
									   std::vector<vec3>&		  tangents,
									   std::vector<vec3>&		  bitangents)
{
	tangents.resize(vertices.size(), vec3(0));
	bitangents.resize(vertices.size(), vec3(0));

	for (size_t i = 0; i < indices.size(); i += 3)
	{
		// Get the indices of the three vertices of the triangle
		unsigned int i1 = indices[i];
		unsigned int i2 = indices[i + 1];
		unsigned int i3 = indices[i + 2];

		// Get the positions of the three vertices
		glm::vec3 v1 = vertices[i1];
		glm::vec3 v2 = vertices[i2];
		glm::vec3 v3 = vertices[i3];

		// Get the UV coordinates of the three vertices
		glm::vec2 uv1 = uvs[i1];
		glm::vec2 uv2 = uvs[i2];
		glm::vec2 uv3 = uvs[i3];

		// Calculate the edge vectors of the triangle
		glm::vec3 edge1 = v2 - v1;
		glm::vec3 edge2 = v3 - v1;

		// Calculate the delta UV coordinates of the triangle
		glm::vec2 deltaUV1 = uv2 - uv1;
		glm::vec2 deltaUV2 = uv3 - uv1;

		// Calculate the tangent and bitangent of the triangle
		float	  f			= 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
		glm::vec3 tangent	= glm::normalize(f * (deltaUV2.y * edge1 - deltaUV1.y * edge2));
		glm::vec3 bitangent = glm::normalize(f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2));

		// Add the tangent and bitangent to the vertices
		tangents[i1] += tangent;
		bitangents[i1] += bitangent;
		tangents[i2] += tangent;
		bitangents[i2] += bitangent;
		tangents[i3] += tangent;
		bitangents[i3] += bitangent;
	}

	// Calculate the average tangent and bitangent for each vertex
	for (size_t i = 0; i < vertices.size(); i++)
	{
		tangents[i]	  = glm::normalize(tangents[i]);
		bitangents[i] = glm::normalize(bitangents[i]);
	}
}

MeshComponent* MeshComponentBuilder::CreateSphere(const std::string& material)
{
	/*auto segments : number = options.segments || 32;
	auto diameterX : number = options.diameterX || options.diameter || 1;
	auto diameterY : number = options.diameterY || options.diameter || 1;
	auto diameterZ : number = options.diameterZ || options.diameter || 1;
	auto arc : number = options.arc && (options.arc <= 0 || options.arc > 1) ? 1.0 : options.arc || 1.0;
	auto slice : number = options.slice && (options.slice <= 0) ? 1.0 : options.slice || 1.0;
	auto sideOrientation = (options.sideOrientation == = 0) ? 0 : options.sideOrientation || VertexData.DEFAULTSIDE;
	auto dedupTopBottomIndices = !!options.dedupTopBottomIndices;*/

	uint32 segments		   = 64;
	float  diameterX	   = 2;
	float  diameterY	   = 2;
	float  diameterZ	   = 2;
	float  arc			   = 1;
	float  slice		   = 1;
	uint32 sideOrientation = 0;

	auto radius = TVector3(diameterX / 2, diameterY / 2, diameterZ / 2);

	auto totalZRotationSteps = 2 + segments;
	auto totalYRotationSteps = 2 * totalZRotationSteps;

	auto indices   = std::vector<uint32>();
	auto positions = std::vector<float>();
	auto colors	   = std::vector<float>();
	auto normals   = std::vector<float>();
	auto uvs	   = std::vector<float>();

	for (auto zRotationStep = 0; zRotationStep <= totalZRotationSteps; zRotationStep++)
	{
		auto normalizedZ = float(zRotationStep) / totalZRotationSteps;
		auto angleZ		 = normalizedZ * PI * slice;

		for (auto yRotationStep = 0; yRotationStep <= totalYRotationSteps; yRotationStep++)
		{
			auto normalizedY = float(yRotationStep) / totalYRotationSteps;

			auto angleY = normalizedY * PI * 2 * arc;

			auto matrix = TMat4x4(1.0f);

			auto rotationZ = glm::rotate(matrix, -angleZ, { 0, 0, 1 });
			auto rotationY = glm::rotate(matrix, angleY, { 0, 1, 0 });
			auto afterRotZ = TVector3(rotationZ * TVector4(0, 1, 0, 1));
			auto complete  = TVector3(rotationY * TVector4(afterRotZ, 1));

			auto vertex = complete * radius;
			auto normal = glm::normalize(complete / radius);

			{
				positions.push_back(vertex.x);
				positions.push_back(vertex.y);
				positions.push_back(vertex.z);
			}
			{
				colors.push_back(1);
				colors.push_back(1);
				colors.push_back(1);
			}
			{
				normals.push_back(normal.x);
				normals.push_back(normal.y);
				normals.push_back(normal.z);
			}
			{
				uvs.push_back(normalizedY);
				uvs.push_back(normalizedZ);
			}
		}

		if (zRotationStep > 0)
		{
			auto verticesCount = positions.size() / 3;
			for (auto firstIndex = verticesCount - 2 * (totalYRotationSteps + 1); (firstIndex + totalYRotationSteps + 2) < verticesCount; firstIndex++)
			{
				indices.push_back(firstIndex);
				indices.push_back(firstIndex + 1);
				indices.push_back(firstIndex + totalYRotationSteps + 1);

				indices.push_back(firstIndex + totalYRotationSteps + 1);
				indices.push_back(firstIndex + 1);
				indices.push_back(firstIndex + totalYRotationSteps + 2);
			}
		}
	}

	MeshComponent* meshComp = new MeshComponent();
	meshComp->Geometry_		= new Geometry;
	meshComp->Material_		= new Material(material.empty() ? "Materials/PBR.json" : material);

	//auto albedoTexture = std::make_shared<Texture2D>("Textures/albedo.png");
	//meshComp->Material_->SetTexture("tAlbedo", albedoTexture);
	//auto matTexture = std::make_shared<Texture2D>("Textures/metallic-roughness.png");
	//meshComp->Material_->SetTexture("tMetallicRoughnessMap", matTexture);
	//auto normalTexture = std::make_shared<Texture2D>("Textures/sphere-normal.png");
	//meshComp->Material_->SetTexture("tNormalMap", normalTexture);
	//auto aoTexture = std::make_shared<Texture2D>("Textures/sphere-ao.png");
	//meshComp->Material_->SetTexture("tAoMap", aoTexture);


	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::POSITION, InputAttributeFormat::FLOAT3, positions.data(), sizeof(positions[0]) * positions.size());
	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::DIFFUSE, InputAttributeFormat::FLOAT3, colors.data(), sizeof(colors[0]) * colors.size());
	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::TEXCOORD, InputAttributeFormat::FLOAT2, uvs.data(), sizeof(uvs[0]) * uvs.size());
	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::NORMAL, InputAttributeFormat::FLOAT3, normals.data(), sizeof(normals[0]) * normals.size());

	std::vector<vec3> tangents;
	std::vector<vec3> bitangents;

	_CalculateTangentBitangent(std::span<vec3>((vec3*)positions.data(), positions.size() / 3),
							   std::span<vec2>((vec2*)uvs.data(), uvs.size() / 2),
							   std::span<uint32>(indices),
							   tangents,
							   bitangents);

	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::TANGENT, InputAttributeFormat::FLOAT3, tangents.data(), sizeof(tangents[0]) * tangents.size());

	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::BITANGENT, InputAttributeFormat::FLOAT3, bitangents.data(), sizeof(bitangents[0]) * bitangents.size());


	meshComp->Geometry_->indexBuffer_.indexType = IndexType::UINT32;
	meshComp->Geometry_->indexBuffer_.InitData(indices.data(), indices.size() * sizeof(indices[0]));

	return meshComp;
}

MeshComponent* MeshComponentBuilder::CreatePlane(const std::string& material)
{
	MeshComponent* meshComp = new MeshComponent();
	meshComp->Geometry_		= new Geometry;
	meshComp->Material_		= new Material(material.empty() ? "Materials/PBR.json" : material);

	std::vector<TVector3> positions = {
		{ -1.0f, 0.0f, -1.0f },
		{ 1.0f, 0.0f, -1.0f },
		{ 1.0f, 0.0f, 1.0f },
		{ -1.0f, 0.0f, 1.0f },
	};

	std::vector<TVector3> colors = {
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },

	};

	std::vector<TVector2> texCoords = {
		{ 0.0f, 0.0f },
		{ 0.0f, 1.0f },
		{ 1.0f, 1.0f },
		{ 1.0f, 0.0f },
		
	};

	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::POSITION, InputAttributeFormat::FLOAT3, positions.data(), sizeof(positions[0]) * positions.size());
	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::DIFFUSE, InputAttributeFormat::FLOAT3, colors.data(), sizeof(colors[0]) * colors.size());
	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::TEXCOORD, InputAttributeFormat::FLOAT2, texCoords.data(), sizeof(texCoords[0]) * texCoords.size());

	std::vector<std::uint32_t> indices = {
		0, 1, 2, 0, 2, 3
	};

	std::vector<TVector3> normals = {
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
	};

	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::NORMAL, InputAttributeFormat::FLOAT3, normals.data(), sizeof(normals[0]) * normals.size());

	meshComp->Geometry_->indexBuffer_.indexType = IndexType::UINT32;
	meshComp->Geometry_->indexBuffer_.InitData(indices.data(), indices.size() * sizeof(indices[0]));


	return meshComp;
}