
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

    //Vector2f GetTextureCoordByVBIndex(uint32 idx);
    template<typename Ty_>
	Ty_ GetAttributeByIndex(VertexBufferAttriKind kind, uint32 idx)
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

    IndexBuffer* GetIndexBuffer()
    {
        return &indexBuffer_;
    }

    bool HasTangent();

    bool HasBiTangent();
    
protected:
    std::vector<VertexBuffer*> vbStreams_;
    IndexBuffer indexBuffer_;

    friend class MeshComponentBuilder;
    friend class Terrain;
};
