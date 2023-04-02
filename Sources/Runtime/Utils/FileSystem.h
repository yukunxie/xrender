#pragma once

#include <vector>
#include <string>

#include "Types.h"


class FileSystem
{
protected:
	FileSystem();

public:
	virtual ~FileSystem() {}

	void AppendSearchPath(const char* searchPath);

public:
	static FileSystem* GetInstance();

	std::string GetAbsFilePath(const char* filename);

	TData GetBinaryData(const char* filename);

	std::string GetStringData(const char* filename);

	static const std::string& GetWorkingDirectory()
	{
		return GetInstance()->WorkingDirectory_;
	}

	static const std::string& GetGameDataDirectory()
	{
		return GetInstance()->GameDataDirectory_;
	}

	static std::string GetDirectory(const std::string& path);

	static bool WriteBinaryData(const char* filename, const TData& data);

	static bool WriteBinaryData(const char* filename, const void* data, std::uint32_t byteLength);

	static bool ReadBinaryData(const char* filename, TData& data);

private:
	static FileSystem* instance_;
	std::string		WorkingDirectory_;
	std::string		GameDataDirectory_;
	std::vector<std::string> searchPaths_;
};
