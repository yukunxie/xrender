

#include "MeshComponent.h"
#include "Object/Entity.h"
#include "Meshes/Geometry.h"
#include "VertexBuffer.h"

#include <glm/ext/matrix_transform.hpp>

#include <math.h>


const float PI = M_PI;

void BindVertexBufferHandle(MeshComponent* meshComp, VertexBufferAttriKind kind, InputAttributeFormat format, const void* data, uint32 size)
{
	auto vbBuffer = new VertexBuffer();
	vbBuffer->kind = kind;
	vbBuffer->format = format;
	vbBuffer->InitData(data, size);
	meshComp->GetGeometry()->AppendVertexBuffer(vbBuffer);
};

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

	//if (RenderObject_.MaterialObject)
	//{
	//	return;
	//}
	//RenderObject_.RenderSetBits = RenderSetBits_;
	//RenderObject_.MaterialObject = Material_;
	//auto& ib = RenderObject_.IndexBuffer;
	//{
	//	ib.GpuBuffer = Geometry_->GetIndexBuffer()->gpuBuffer;
	//	ib.IndexCount = Geometry_->GetIndexBuffer()->GetIndexCount();
	//	ib.InstanceCount = 0;
	//	ib.IndexType = IndexType::UINT32;
	//	ib.Offset = 0;
	//}

	////InputAssembler 

	//std::vector<InputAttribute> attributes;

	//RenderObject_.VertexBuffers.clear();

	//uint32 slot = 0;

	//for (auto& vbs : Geometry_->GetVBStreams())
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

	//Material_->SetInputAssembler({ attributes , IndexType::UINT32 });
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
		const auto& matrix = TMat4x4(1.0f);;
		Material_->SetFloat("WorldMatrix", 0, 16, (float*)&matrix);
	}

	//Engine::GetRenderScene()->AddRenderObject(&RenderObject_);
}

MeshComponent* MeshComponentBuilder::CreateBox(const std::string& material)
{
	MeshComponent* meshComp = new MeshComponent();
	meshComp->Geometry_ = new Geometry;
	meshComp->Material_ = new Material(material.empty() ? "Materials/PBR.json" : material);

	std::vector<TVector3> positions = {
		// Front face
		{-1.0f, -1.0f, 1.0f},
		{1.0f,  -1.0f, 1.0f},
		{1.0f,  1.0f,  1.0f},
		{-1.0f, 1.0f,  1.0f},
		// Back face
		{-1.0f, -1.0f, -1.0f},
		{-1.0f, 1.0f,  -1.0f},
		{1.0f,  1.0f,  -1.0f},
		{1.0f,  -1.0f, -1.0f},
		// Top face
		{-1.0f, 1.0f,  -1.0f},
		{-1.0f, 1.0f,  1.0f},
		{1.0f,  1.0f,  1.0f},
		{1.0f,  1.0f,  -1.0f},
		// Bottom face
		{-1.0f, -1.0f, -1.0f},
		{1.0f,  -1.0f, -1.0f},
		{1.0f,  -1.0f, 1.0f},
		{-1.0f, -1.0f, 1.0f},
		// Right face
		{1.0f,  -1.0f, -1.0f},
		{1.0f,  1.0f,  -1.0f},
		{1.0f,  1.0f,  1.0f},
		{1.0f,  -1.0f, 1.0f},
		// Left face
		{-1.0f, -1.0f, -1.0f},
		{-1.0f, -1.0f, 1.0f},
		{-1.0f, 1.0f,  1.0f},
		{-1.0f, 1.0f,  -1.0f},
	};

	std::vector<TVector3> colors = {
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
	};

	std::vector<TVector2> texCoords =
	{
		{1.0f, 0.0f},
		{0.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 1.0f},
		{1.0f, 0.0f},
		{0.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 1.0f},
		{1.0f, 0.0f},
		{0.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 1.0f},
		{1.0f, 0.0f},
		{0.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 1.0f},
		{1.0f, 0.0f},
		{0.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 1.0f},
		{1.0f, 0.0f},
		{0.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 1.0f}
	};

	

	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::POSITION, InputAttributeFormat::FLOAT3, positions.data(), sizeof(positions[0]) * positions.size());
	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::DIFFUSE, InputAttributeFormat::FLOAT3, colors.data(), sizeof(colors[0])* colors.size());
	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::TEXCOORD, InputAttributeFormat::FLOAT2, texCoords.data(), sizeof(texCoords[0])* texCoords.size());

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
		uint32 idx = face * 6;
		const TVector3& v0 = positions[indices[idx]];
		const TVector3& v1 = positions[indices[idx + 1]];
		const TVector3& v2 = positions[indices[idx + 2]];

		TVector3 d0 = v1 - v0;
		TVector3 d1 = v2 - v0;

		TVector3 normal = -glm::normalize(glm::cross(d1, d0));

		for (uint32 i = 0; i < 6; i++)
		{
			normals[indices[idx + i]] = normal;
		}
	}
	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::NORMAL, InputAttributeFormat::FLOAT3, normals.data(), sizeof(normals[0])* normals.size());

	meshComp->Geometry_->indexBuffer_.indexType = IndexType::UINT32;
	meshComp->Geometry_->indexBuffer_.InitData(indices.data(), indices.size() * sizeof(indices[0]));
	

	return meshComp;
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

	uint32 segments = 32;
	float diameterX = 2;
	float diameterY = 2;
	float diameterZ = 2; 
	float arc = 1;
	float slice = 1;
	uint32 sideOrientation = 0;

	auto radius = TVector3(diameterX / 2, diameterY / 2, diameterZ / 2);

	auto totalZRotationSteps = 2 + segments;
	auto totalYRotationSteps = 2 * totalZRotationSteps;

	auto indices = std::vector<uint32>();
	auto positions = std::vector<float>();
	auto colors = std::vector<float>();
	auto normals = std::vector<float>();
	auto uvs = std::vector<float>();

	for (auto zRotationStep = 0; zRotationStep <= totalZRotationSteps; zRotationStep++) {
		auto normalizedZ = float(zRotationStep) / totalZRotationSteps;
		auto angleZ = normalizedZ * PI * slice;

		for (auto yRotationStep = 0; yRotationStep <= totalYRotationSteps; yRotationStep++) {
			auto normalizedY = float(yRotationStep) / totalYRotationSteps;

			auto angleY = normalizedY * PI * 2 * arc;

			auto matrix = TMat4x4(1.0f);

			auto rotationZ = glm::rotate(matrix, -angleZ, { 0, 0, 1 });
			auto rotationY = glm::rotate(matrix, angleY, { 0, 1, 0 });
			auto afterRotZ = TVector3(rotationZ * TVector4(0, 1, 0, 1));
			auto complete = TVector3(rotationY * TVector4(afterRotZ, 1));

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

		if (zRotationStep > 0) {
			auto verticesCount = positions.size() / 3;
			for (auto firstIndex = verticesCount - 2 * (totalYRotationSteps + 1); (firstIndex + totalYRotationSteps + 2) < verticesCount; firstIndex++) {
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
	meshComp->Geometry_ = new Geometry;
	meshComp->Material_ = new Material(material.empty() ? "Materials/PBR.json" : material);

	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::POSITION, InputAttributeFormat::FLOAT3, positions.data(), sizeof(positions[0]) * positions.size());
	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::DIFFUSE, InputAttributeFormat::FLOAT3, colors.data(), sizeof(colors[0]) * colors.size());
	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::TEXCOORD, InputAttributeFormat::FLOAT2, uvs.data(), sizeof(uvs[0]) * uvs.size());
	BindVertexBufferHandle(meshComp, VertexBufferAttriKind::NORMAL, InputAttributeFormat::FLOAT3, normals.data(), sizeof(normals[0]) * normals.size());

	meshComp->Geometry_->indexBuffer_.indexType = IndexType::UINT32;
	meshComp->Geometry_->indexBuffer_.InitData(indices.data(), indices.size() * sizeof(indices[0]));

	return meshComp;
}