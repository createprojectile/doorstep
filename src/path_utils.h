#pragma once
#include <string>

std::wstring GetExePathW();
std::wstring GetDirName(const std::wstring &path);
bool IsAbsolutePath(const std::wstring &p);
std::wstring PathJoin(const std::wstring &a, const std::wstring &b);
std::wstring ResolveAgainstExe(const std::wstring &exeDir, const std::wstring &rel);
bool FileExists(const std::wstring &p);
std::string ToUtf8(const std::wstring &w);
