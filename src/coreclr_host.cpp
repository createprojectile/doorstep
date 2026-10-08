#include "coreclr_host.h"
#include "config.h"
#include "globals.h"
#include "logger.h"
#include "path_utils.h"
#include <windows.h>
#include <string>

using coreclr_initialize_fn = HRESULT (*)(
    const char *exePath,
    const char *appDomainFriendlyName,
    int propertyCount,
    const char **propertyKeys,
    const char **propertyValues,
    void **hostHandle,
    unsigned int *domainId
);

using coreclr_create_delegate_fn = HRESULT (*)(
    void *hostHandle,
    unsigned int domainId,
    const char *entryPointAssemblyName,
    const char *entryPointTypeName,
    const char *entryPointMethodName,
    void **delegate
);

void SetDoorstopEnvVars()
{
    std::wstring searchDirs = g_dotnetFull + L";" + g_bepInExCoreFull;

    SetEnvironmentVariableW(L"DOORSTOP_INITIALIZED", L"TRUE");
    SetEnvironmentVariableW(L"DOORSTOP_PROCESS_PATH", g_exePath.c_str());
    SetEnvironmentVariableW(L"DOORSTOP_INVOKE_DLL_PATH", g_targetAsmFull.c_str());
    SetEnvironmentVariableW(L"DOORSTOP_MANAGED_FOLDER_DIR", g_dotnetFull.c_str());
    SetEnvironmentVariableW(L"DOORSTOP_DLL_SEARCH_DIRS", searchDirs.c_str());
}

bool RunCoreClrBootstrap()
{
    std::wstring coreclrFull = PathJoin(g_dotnetFull, L"coreclr.dll");
    Log(L"coreclr  : %ls", coreclrFull.c_str());

    if (!FileExists(coreclrFull))
    {
        Log(L"ERROR: coreclr.dll not found at %ls", coreclrFull.c_str());
        return false;
    }

    HMODULE hClr = LoadLibraryW(coreclrFull.c_str());
    if (!hClr)
    {
        Log(L"ERROR: LoadLibraryW(coreclr) -> %lu", GetLastError());
        return false;
    }

    auto coreclr_initialize = (coreclr_initialize_fn)GetProcAddress(hClr, "coreclr_initialize");
    auto coreclr_create_delegate = (coreclr_create_delegate_fn)GetProcAddress(hClr, "coreclr_create_delegate");

    if (!coreclr_initialize || !coreclr_create_delegate)
    {
        Log(L"ERROR: coreclr.dll lacks coreclr_initialize / coreclr_create_delegate");
        return false;
    }

    std::string appPaths = ToUtf8(g_dotnetFull) + ";" + ToUtf8(g_bepInExCoreFull);
    const char *keys[] = {"APP_PATHS"};
    const char *values[] = {appPaths.c_str()};
    constexpr int kPropCount = 1;

    std::string exePathU8 = ToUtf8(g_exePath);

    void *hostHandle = nullptr;
    unsigned int domainId = 0;

    Log(L"coreclr_initialize(APP_PATHS=%hs)", appPaths.c_str());
    HRESULT hr =
        coreclr_initialize(exePathU8.c_str(), "Doorstop Domain", kPropCount, keys, values, &hostHandle, &domainId);
    if (FAILED(hr) || hostHandle == nullptr)
    {
        Log(L"ERROR: coreclr_initialize -> 0x%08lX", (unsigned long)hr);
        return false;
    }
    Log(L"CoreCLR started. domainId=%u", domainId);

    void *fn = nullptr;
    hr = coreclr_create_delegate(
        hostHandle, domainId, cfg::kTargetAssemblyName, cfg::kEntryType, cfg::kEntryMethod, &fn
    );
    if (FAILED(hr) || fn == nullptr)
    {
        Log(L"ERROR: coreclr_create_delegate -> 0x%08lX", (unsigned long)hr);
        return false;
    }

    Log(L"Invoking %hs::%hs ...", cfg::kEntryType, cfg::kEntryMethod);
    using EntryFn = void (*)();
    ((EntryFn)fn)();
    Log(L"Managed bootstrap returned; BepInEx is live.");

    return true;
}
