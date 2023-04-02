#include "Types.h"
#include <stdarg.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <fstream>

#include <winnt.h>


static constexpr std::uint32_t _SimpleHash(const char* p, uint32 key = 31)
{
	std::uint32_t hash = 0;
	for (; *p; p++)
	{
		hash = hash * key + *p;
	}
	return hash;
}

MaterialParameterType ToMaterialParameterType(const char* format)
{
	switch (_SimpleHash(format))
	{
	case _SimpleHash("float"):
		return MaterialParameterType::FLOAT;
	case _SimpleHash("float2"):
		return MaterialParameterType::FLOAT2;
	case _SimpleHash("float3"):
		return MaterialParameterType::FLOAT3;
	case _SimpleHash("float4"):
		return MaterialParameterType::FLOAT4;
	case _SimpleHash("int"):
		return MaterialParameterType::INT;
	case _SimpleHash("int2"):
		return MaterialParameterType::INT2;
	case _SimpleHash("int3"):
		return MaterialParameterType::INT3;
	case _SimpleHash("int4"):
		return MaterialParameterType::INT4;
	case _SimpleHash("bool"):
		return MaterialParameterType::BOOL;
	case _SimpleHash("bool1"):
		return MaterialParameterType::BOOL1;
	case _SimpleHash("bool2"):
		return MaterialParameterType::BOOL2;
	case _SimpleHash("bool3"):
		return MaterialParameterType::BOOL3;
	case _SimpleHash("mat4"):
		return MaterialParameterType::MAT4;
	case _SimpleHash("mat3"):
		return MaterialParameterType::MAT3;
	case _SimpleHash("mat2"):
		return MaterialParameterType::MAT2;
	case _SimpleHash("mat4x3"):
		return MaterialParameterType::MAT4x3;
	case _SimpleHash("mat4x2"):
		return MaterialParameterType::MAT4x2;
	case _SimpleHash("mat3x4"):
		return MaterialParameterType::MAT3x4;
	case _SimpleHash("mat2x4"):
		return MaterialParameterType::MAT2x4;
	case _SimpleHash("mat3x2"):
		return MaterialParameterType::MAT3x2;
	case _SimpleHash("Buffer"):
		return MaterialParameterType::BUFFER;
	case _SimpleHash("Sampler2D"):
		return MaterialParameterType::SAMPLER2D;
	case _SimpleHash("Texture2D"):
		return MaterialParameterType::TEXTURE2D;
	default:
		Assert(false, "invalid format");
	}
}

std::uint32_t GetInputAttributeLocation(VertexBufferAttriKind kind)
{
	switch (kind)
	{
	case VertexBufferAttriKind::POSITION:
		return IA_LOCATION_POSITION;
	case VertexBufferAttriKind::NORMAL:
		return IA_LOCATION_NORMAL;
	case VertexBufferAttriKind::TEXCOORD:
		return IA_LOCATION_TEXCOORD;
	case VertexBufferAttriKind::DIFFUSE:
		return IA_LOCATION_DIFFUSE;
	case VertexBufferAttriKind::TANGENT:
		return IA_LOCATION_TANGENT;
	case VertexBufferAttriKind::BINORMAL:
		return IA_LOCATION_BINORMAL;
	case VertexBufferAttriKind::BITANGENT:
		return IA_LOCATION_BITANGENT;
	case VertexBufferAttriKind::TEXCOORD2:
		return IA_LOCATION_TEXCOORD2;
	}
	Assert(false, "invalid kind");
}

std::uint32_t GetFormatSize(InputAttributeFormat format)
{
	switch (format)
	{
	case InputAttributeFormat::FLOAT:
		return 4;
	case InputAttributeFormat::FLOAT2:
		return 8;
	case InputAttributeFormat::FLOAT3:
		return 12;
	case InputAttributeFormat::FLOAT4:
		return 16;
	}
	Assert(false, "");
	return 0;
}

//   0 = 黑色      8 = 灰色
//   1 = 蓝色      9 = 淡蓝色
//   2 = 绿色      A = 淡绿色
//   3 = 浅绿色    B = 淡浅绿色
//   4 = 红色      C = 淡红色
//   5 = 紫色      D = 淡紫色
//   6 = 黄色      E = 淡黄色
//   7 = 白色      F = 亮白色

//   控制台前景颜色
enum ConsoleForegroundColor
{
	enmCFC_Red		 = FOREGROUND_INTENSITY | FOREGROUND_RED,
	enmCFC_Green	 = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
	enmCFC_Blue		 = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
	enmCFC_Yellow	 = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
	enmCFC_Purple	 = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
	enmCFC_Cyan		 = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
	enmCFC_Gray		 = FOREGROUND_INTENSITY,
	enmCFC_White	 = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	enmCFC_HighWhite = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
	enmCFC_Black	 = 0,
};

enum ConsoleBackGroundColor
{
	enmCBC_Red		 = BACKGROUND_INTENSITY | BACKGROUND_RED,
	enmCBC_Green	 = BACKGROUND_INTENSITY | BACKGROUND_GREEN,
	enmCBC_Blue		 = BACKGROUND_INTENSITY | BACKGROUND_BLUE,
	enmCBC_Yellow	 = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN,
	enmCBC_Purple	 = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE,
	enmCBC_Cyan		 = BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE,
	enmCBC_White	 = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
	enmCBC_HighWhite = BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
	enmCBC_Black	 = 0,
};

void SetConsoleColor(ConsoleForegroundColor foreColor = enmCFC_White, ConsoleBackGroundColor backColor = enmCBC_Black)
{
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(handle, foreColor | backColor);
}

void _OutputLog(LogLevel level, const char* format, ...)
{
	va_list vaList; // equal to Format + sizeof(FOrmat)
	char	szBuff[10240] = { 0 };
	va_start(vaList, format);
	vsnprintf(szBuff, sizeof(szBuff), format, vaList);
	va_end(vaList);

	std::stringstream logger;

	auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	logger << std::put_time(std::localtime(&t), "%Y-%m-%d %X ") << szBuff << std::endl;

	//	static std::ofstream _FileLogger;
	//	if (!_FileLogger.is_open())
	//	{
	//		auto			  t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	//		std::stringstream ss;
	//		ss << std::put_time(std::localtime(&t), "%Y%m%d-%H%M%S");
	//		std::string dir		 = "Log/";
	//		std::string filename = dir + ss.str() + ".log";
	//
	//#if WIN32
	//		std::wstring wdir;
	//		{
	//			//获取缓冲区大小，并申请空间，缓冲区大小按字符计算
	//			int	   len	  = MultiByteToWideChar(CP_ACP, 0, dir.c_str(), dir.size(), NULL, 0);
	//			WCHAR* buffer = new WCHAR[len + 1];
	//			//多字节编码转换成宽字节编码
	//			MultiByteToWideChar(CP_ACP, 0, dir.c_str(), dir.size(), buffer, len);
	//			buffer[len] = '\0';
	//			//删除缓冲区并返回值
	//			wdir.append(buffer);
	//			delete[] buffer;
	//		}
	//
	//		auto flag = GetFileAttributesA(dir.c_str());
	//
	//		if (INVALID_FILE_ATTRIBUTES == flag || (!(flag & FILE_ATTRIBUTE_DIRECTORY)))
	//		{
	//			bool flag = CreateDirectory(wdir.c_str(), NULL);
	//			Assert(flag, "create 'Log/' dir fail.");
	//		}
	//#else
	//		Assert(false, "");
	//#endif
	//		_FileLogger.open(filename, std::ios::out);
	//	}

	switch (level)
	{
	case LogLevel::Info:
		SetConsoleColor(ConsoleForegroundColor::enmCFC_White);
		break;
	case LogLevel::Warning:
		SetConsoleColor(ConsoleForegroundColor::enmCFC_Yellow);
		break;
	case LogLevel::Error:
		SetConsoleColor(ConsoleForegroundColor::enmCFC_Red);
		break;
	default:
		SetConsoleColor(ConsoleForegroundColor::enmCFC_Gray);
	}

	std::cout << logger.str();
	//_FileLogger << logger.str();
	//_FileLogger.flush();
}

void AssertImpl(const char* filename, const char* function, int lineno, const char* format, ...)
{
	if (true)
		;
}


void* alignedMalloc(size_t size, size_t align)
{
	if (size == 0)
		return nullptr;

	assert((align & (align - 1)) == 0);
	void* ptr = _mm_malloc(size, align);
	if (size != 0 && ptr == nullptr)
		throw std::bad_alloc();
	return ptr;
}

void alignedFree(void* ptr)
{
	if (ptr)
		_mm_free(ptr);
}