
#include "Geometry.h"

#include <memory>

template<>
bool CheckReturnTypeAndFormatMatched<float, InputAttributeFormat::FLOAT>()
{
	return true;
}

template<>
bool CheckReturnTypeAndFormatMatched<Vector2f, InputAttributeFormat::FLOAT2>()
{
	return true;
}

template<>
bool CheckReturnTypeAndFormatMatched<Vector3f, InputAttributeFormat::FLOAT3>()
{
	return true;
}

template<>
bool CheckReturnTypeAndFormatMatched<Vector4f, InputAttributeFormat::FLOAT4>()
{
	return true;
}

Geometry::Geometry()
{
}

void Geometry::AppendVertexBuffer(VertexBuffer* buffer)
{
    vbStreams_.push_back(buffer);
}

//Vector2f Geometry::GetTextureCoordByVBIndex(uint32 idx)
//{
//	for (auto vb: vbStreams_)
//	{
//		if (vb->kind == VertexBufferAttriKind::TEXCOORD)
//		{
//			Assert(vb->format == InputAttributeFormat::FLOAT2);
//			return ((Vector2f*)(vb->buffer.data()))[idx];
//        }
//    }
//	return { 0.0f, 0.0f };
//}

bool Geometry::HasTangent() const
{
    for (const auto& vb : vbStreams_)
    {
        if (vb->kind == VertexBufferAttriKind::TANGENT)
            return true;
    }
    return false;
}

bool Geometry::HasBiTangent() const
{
    for (const auto& vb : vbStreams_)
    {
        if (vb->kind == VertexBufferAttriKind::BITANGENT)
            return true;
    }
    return false;
}