//
// Created by realxie on 2019-10-29.
//

#include "Entity.h"
#include "Component.h"
#include "MeshComponent.h"

Entity::Entity()
    : WorldMatrix_(1.0f)
{
}

Entity::~Entity()
{
}

void Entity::AddComponment(Component* componment)
{
    Components_.emplace_back(componment);
    componment->SetEntity(this);
}

void Entity::AddChild(Entity* child)
{
    auto it = std::find(Children_.begin(), Children_.end(), child);
    if (it != Children_.end())
        return;
    Children_.push_back(child);
    child->Parent_ = this;
}

void Entity::RemoveChild(Entity* child)
{
    auto it = std::find(Children_.begin(), Children_.end(), child);
    if (it != Children_.end())
    {
        Children_.erase(it);
        child->Parent_ = nullptr;
    }
}

void Entity::SetParent(Entity* parent)
{
    if (Parent_ && Parent_ != parent)
    {
        Parent_->RemoveChild(this);
    }

    if (parent)
        parent->AddChild(this);
    else
        Parent_ = nullptr;
}

void Entity::UpdateWorldMatrix() const
{
    if (!IsTransformDirty_)
        return;

    IsTransformDirty_ = false;

    WorldMatrix_ = glm::mat4(1);

    //translate
    WorldMatrix_ = glm::translate(WorldMatrix_, Transform_.Position());

    WorldMatrix_ = glm::scale(WorldMatrix_, Transform_.Scale());

    WorldMatrix_ = glm::rotate(WorldMatrix_, glm::radians(Transform_.Rotation().x), TVector3(1.0f, 0.0f, 0.0f));
    WorldMatrix_ = glm::rotate(WorldMatrix_, glm::radians(Transform_.Rotation().y), TVector3(0.0f, 1.0f, 0.0f));
    WorldMatrix_ = glm::rotate(WorldMatrix_, glm::radians(Transform_.Rotation().z), TVector3(0.0f, 0.0f, 1.0f));
}

void Entity::Tick(float dt)
{
    for (auto& cm : Components_)
    {
        cm->Tick(dt);
    }

    for (auto child : Children_)
    {
        child->Tick(dt);
    }
}
extern std::map<int, MeshComponent*> GMeshComponentProxies;

void Entity::AddToEmbreeScene(RTCDevice device_i, RTCScene scene_i)
{
	for (auto child : Children_)
	{
		if (child)
		{
			child->AddToEmbreeScene(device_i, scene_i);
		}
	}

	for (auto component : Components_)
	{
		auto meshComponent = dynamic_cast<MeshComponent*>(component);
		if (!meshComponent || !meshComponent->GetGeometry())
		{
			continue;
		}
		auto geometry = meshComponent->GetGeometry();

		RTCGeometry rtcMesh = rtcNewGeometry(device_i, RTC_GEOMETRY_TYPE_TRIANGLE);

		std::vector<Vector3f>	  sphereVertices;
		std::vector<Vector3f>	  sphereNormals;
		std::vector<Vector2f>	  sphereUVs;
		std::vector<unsigned int> indices;

		for (auto vb : geometry->GetVBStreams())
		{
			if (vb->kind == VertexBufferAttriKind::POSITION)
			{
				RTCFormat format	 = RTC_FORMAT_FLOAT3;
				size_t	  byteStride = 0;
				if (vb->format == InputAttributeFormat::FLOAT2)
				{
					Assert(false);
					format	   = RTC_FORMAT_FLOAT2;
					byteStride = 2 * sizeof(float);
				}
				else if (vb->format == InputAttributeFormat::FLOAT3)
				{
					format	   = RTC_FORMAT_FLOAT3;
					byteStride = 3 * sizeof(float);
				}
				else if (vb->format == InputAttributeFormat::FLOAT4)
				{
					Assert(false);
					format	   = RTC_FORMAT_FLOAT4;
					byteStride = 4 * sizeof(float);
				}
				else
				{
					Assert(false);
				}

				void* vert = (void*)rtcSetNewGeometryBuffer(rtcMesh, RTC_BUFFER_TYPE_VERTEX, 0, format, byteStride, vb->buffer.size() / byteStride);
				memcpy(vert, vb->buffer.data(), vb->buffer.size());
			}
		}

		auto ib = geometry->GetIndexBuffer();
		if (ib->indexType == IndexType::UINT16)
		{
			uint16* ptr = (uint16*)ib->buffer.data();
			for (uint32 i = 0; i < ib->GetIndexCount(); ++i)
			{
				indices.push_back(ptr[i]);
			}
		}
		else
		{
			uint32* ptr = (uint32*)ib->buffer.data();
			for (uint32 i = 0; i < ib->GetIndexCount(); ++i)
			{
				indices.push_back(ptr[i]);
			}
		}

		unsigned int* index = (unsigned int*)rtcSetNewGeometryBuffer(rtcMesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned int), indices.size() / 3);
		memcpy(index, indices.data(), sizeof(indices[0]) * indices.size());

		rtcCommitGeometry(rtcMesh);

		unsigned int geomID = rtcAttachGeometry(scene_i, rtcMesh);

		std::cout << "AddEntityToEmbreeScene geomID=" << geomID << std::endl;

		GMeshComponentProxies[geomID] = meshComponent;
	}

	
    
}