#include "logger.h"
#include "config.h"
#include "path_utils.h"
#include <windows.h>
#include <cstdarg>
#include <cstdio>

static std::wstring g_logPath;

void LogInit(const std::wstring &exeDir)
{
    if (!cfg::kEnableLogging)
        return;
    g_logPath = PathJoin(exeDir, L"doorstop.log");
    FILE *f = nullptr;
    if (_wfopen_s(&f, g_logPath.c_str(), L"w, ccs=UTF-8") == 0 && f)
        fclose(f);
}

void Log(const wchar_t *fmt, ...)
{
    if (!cfg::kEnableLogging || g_logPath.empty())
        return;
    FILE *f = nullptr;
    if (_wfopen_s(&f, g_logPath.c_str(), L"a, ccs=UTF-8") != 0 || !f)
        return;

    SYSTEMTIME st {};
    GetLocalTime(&st);
    fwprintf(f, L"[%04d-%02d-%02d %02d:%02d:%02d] ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    va_list ap;
    va_start(ap, fmt);
    vfwprintf(f, fmt, ap);
    va_end(ap);
    fputws(L"\n", f);
    fclose(f);
}

void LogErrorBox(const wchar_t *msg)
{
    Log(L"FATAL: %ls", msg);
    MessageBoxW(nullptr, msg, L"Doorstop Launcher", MB_ICONERROR);
}
