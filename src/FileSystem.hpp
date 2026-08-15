#pragma once

#include <string>

#include "inttypes.hpp"

extern u32 g_LastFileSize;

namespace FileSystem
{
i32 CheckFileExists(const char *file);
u8 *OpenFile(const char *filepath, i32 isExternalResource);
i32 WriteDataToFile(const char *filename, const void *out, u32 bytesToWrite);
std::string GetBasePath(const char *filename);
std::string GetPrefPath(const char *filename);
} // namespace FileSystem
