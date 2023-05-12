#pragma once
#include "Types.h"

struct VertexBuffer
{
	VertexBufferAttriKind kind = VertexBufferAttriKind::INVALID;
	InputAttributeFormat  format;
	std::uint32_t		  byteStride = 0;

	TData		   buffer;

	void InitData(const void* data, std::uint32_t size)
	{
		if (buffer.size() >= size)
		{
			memcpy(buffer.data(), data, size);
		}
		else
		{
			buffer.resize(size);
			memcpy(buffer.data(), data, size);
		}
	}

public:
	static VertexBufferAttriKind NameToVBAttrKind(const std::string& name);
};

struct IndexBuffer
{
	IndexType	   indexType = IndexType::UINT32;
	TData		   buffer;

	void InitData(const void* data, std::uint32_t size)
	{
		if (buffer.size() >= size)
		{
			memcpy(buffer.data(), data, size);
		}
		else
		{
			buffer.resize(size);
			memcpy(buffer.data(), data, size);
		}
	}

	std::uint32_t GetIndexCount() const
	{
		return buffer.size() / (indexType == IndexType::UINT32 ? 4 : 2);
	}

	std::tuple<uint32, uint32, uint32> GetVerticesByPrimitiveId(uint32 primId) const
	{
		if (indexType == IndexType::UINT32)
		{
			const Triangle* triangles = (const Triangle*)buffer.data();
			return { triangles[primId].v0, triangles[primId].v1, triangles[primId].v2 };
		}
		else
		{
			const TriangleUint16* triangles = (const TriangleUint16*)buffer.data();
			return { triangles[primId].v0, triangles[primId].v1, triangles[primId].v2 };
		}
	}
};