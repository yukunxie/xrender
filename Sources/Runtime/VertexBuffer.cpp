#include "VertexBuffer.h"

static constexpr std::uint32_t _SimpleHash(const char* p)
{
	std::uint32_t hash = 0;
	for (; *p; p++)
	{
		hash = hash * 31 + *p;
	}
	return hash;
}

VertexBufferAttriKind VertexBuffer::NameToVBAttrKind(const std::string& name)
{
	switch (_SimpleHash(name.c_str()))
	{
	case _SimpleHash("POSITION"):
		return VertexBufferAttriKind::POSITION;
	case _SimpleHash("DIFFUSE"):
		return VertexBufferAttriKind::DIFFUSE;
	case _SimpleHash("NORMAL"):
		return VertexBufferAttriKind::NORMAL;
	case _SimpleHash("TEXCOORD"):
	case _SimpleHash("TEXCOORD_0"):
		return VertexBufferAttriKind::TEXCOORD;
	case _SimpleHash("TANGENT"):
		return VertexBufferAttriKind::TANGENT;
	case _SimpleHash("BINORMAL"):
		return VertexBufferAttriKind::BINORMAL;
	case _SimpleHash("BITANGENT"):
		return VertexBufferAttriKind::BITANGENT;
	case _SimpleHash("TEXCOORD_1"):
	case _SimpleHash("TEXCOORD1"):
		return VertexBufferAttriKind::TEXCOORD1;
	case _SimpleHash("TEXCOORD_2"):
	case _SimpleHash("TEXCOORD2"):
		return VertexBufferAttriKind::TEXCOORD2;
	default:
		Assert(false, "Invalid vertex input attribute");
	}
	return VertexBufferAttriKind::INVALID;
}