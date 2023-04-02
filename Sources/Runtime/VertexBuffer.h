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

	std::uint32_t GetIndexCount()
	{
		return buffer.size() / (indexType == IndexType::UINT32 ? 4 : 2);
	}

	std::tuple<uint32, uint32, uint32> GetVerticesByPrimitiveId(uint32 primId)
	{
		if (indexType == IndexType::UINT32)
		{
			auto* triangles = (std::tuple<uint32, uint32, uint32>*)buffer.data();
			return triangles[primId];
		}
		else
		{
			auto* triangles = (std::tuple<uint16, uint16, uint16>*)buffer.data();
			return triangles[primId];
		}
	}
};