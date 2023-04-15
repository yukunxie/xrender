
#pragma once

#include "Types.h"

#include "VertexBuffer.h"


class MeshComponentBuilder;
class Terrain;

template<typename VectorType, InputAttributeFormat format = InputAttributeFormat::FLOAT>
bool CheckReturnTypeAndFormatMatched()
{
	return false;
}

class Geometry
{
public:
    Geometry();
    
    void AppendVertexBuffer(VertexBuffer* buffer);
    
    const std::vector<VertexBuffer*>& GetVBStreams()
    {
        return vbStreams_;
    }

	bool HasAttribute(VertexBufferAttriKind kind) const
	{
		for (auto vb : vbStreams_)
		{
			if (vb->kind == kind)
			{
				return true;
			}
		}
		return false;
	}

    //Vector2f GetTextureCoordByVBIndex(uint32 idx);
    template<typename Ty_>
	Ty_ GetAttributeByIndex(VertexBufferAttriKind kind, uint32 idx) const
	{
		for (auto vb : vbStreams_)
		{
			if (vb->kind == kind)
			{
				std::span<Ty_> view = { (Ty_*)vb->buffer.data(), vb->buffer.size() / sizeof(Ty_) };
				return view[idx];
			}
		}
		return Ty_{};
    }

    template<typename Ty_>
	std::tuple<Ty_, Ty_, Ty_> GetTripleAttributesByIndex(VertexBufferAttriKind kind, uint32 idx0, uint32 idx1, uint32 idx2) const
	{
		for (auto vb : vbStreams_)
		{
			if (vb->kind == kind)
			{
				std::span<Ty_> view = { (Ty_*)vb->buffer.data(), vb->buffer.size() / sizeof(Ty_) };
				return { view[idx0], view[idx1], view[idx2] };
			}
		}
		return { Ty_{}, Ty_{}, Ty_{} };
	}

    const IndexBuffer* GetIndexBuffer() const 
    {
        return &indexBuffer_;
    }

	void InitIndexBufferData(const void* data, std::uint32_t size)
	{
		indexBuffer_.InitData(data, size);
	}

    bool HasTangent() const;

    bool HasBiTangent() const;
    
protected:
    std::vector<VertexBuffer*> vbStreams_;
    IndexBuffer indexBuffer_;

    friend class MeshComponentBuilder;
    friend class Terrain;
};
