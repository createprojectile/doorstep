#include "path_utils.h"
#include <windows.h>

std::wstring GetExePathW()
{
    wchar_t buf[MAX_PATH * 2];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH * 2);
    return (n == 0) ? std::wstring() : std::wstring(buf, n);
}

std::wstring GetDirName(const std::wstring &path)
{
    size_t pos = path.find_last_of(L"\\/");
    return (pos == std::wstring::npos) ? std::wstring() : path.substr(0, pos);
}

bool IsAbsolutePath(const std::wstring &p)
{
    if (p.size() >= 3 && p[1] == L':' && (p[2] == L'\\' || p[2] == L'/'))
        return true;
    if (p.size() >= 2 && p[0] == L'\\' && p[1] == L'\\')
        return true;
    return false;
}

std::wstring PathJoin(const std::wstring &a, const std::wstring &b)
{
    if (a.empty())
        return b;
    if (b.empty())
        return a;
    wchar_t last = a.back();
    if (last == L'\\' || last == L'/')
        return a + b;
    return a + L"\\" + b;
}

std::wstring ResolveAgainstExe(const std::wstring &exeDir, const std::wstring &rel)
{
    return IsAbsolutePath(rel) ? rel : PathJoin(exeDir, rel);
}

bool FileExists(const std::wstring &p)
{
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

std::string ToUtf8(const std::wstring &w)
{
    if (w.empty())
        return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    if (n <= 0)
        return std::string();
    std::string s((size_t)n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}
