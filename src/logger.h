#pragma once
#include <string>

void LogInit(const std::wstring &exeDir);
void Log(const wchar_t *fmt, ...);
void LogErrorBox(const wchar_t *msg);
