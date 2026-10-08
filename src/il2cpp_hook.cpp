#include "il2cpp_hook.h"
#include "config.h"
#include "coreclr_host.h"
#include "logger.h"
#include <minhook/minhook.h>

using il2cpp_init_fn = int (*)(const char *domain_name);
static il2cpp_init_fn g_realIl2cppInit = nullptr;
static bool g_bootstrapDone = false;

static int DetourIl2cppInit(const char *domain_name)
{
    Log(L"il2cpp_init detour entered (domain='%hs')", domain_name ? domain_name : "<null>");

    int result = g_realIl2cppInit ? g_realIl2cppInit(domain_name) : -1;
    Log(L"real il2cpp_init returned %d", result);

    if (!g_bootstrapDone)
    {
        g_bootstrapDone = true;
        Log(L"Running deferred CoreCLR bootstrap...");
        if (!RunCoreClrBootstrap())
        {
            Log(L"CoreCLR bootstrap failed.");
            if (!cfg::kContinueOnManagedFailure)
                LogErrorBox(L"BepInEx bootstrap failed. See doorstop.log.");
        }
    }

    return result;
}

bool InstallIl2CppInitHook(HMODULE hGameAssembly)
{
    if (!hGameAssembly)
    {
        Log(L"ERROR: GameAssembly.dll module handle is null.");
        return false;
    }

    void *target = (void *)GetProcAddress(hGameAssembly, cfg::kIl2CppInitExport);
    if (!target)
    {
        Log(L"ERROR: '%hs' not exported by GameAssembly.dll", cfg::kIl2CppInitExport);
        return false;
    }
    Log(L"il2cpp_init target: %p", target);

    MH_STATUS st = MH_Initialize();
    if (st != MH_OK && st != MH_ERROR_ALREADY_INITIALIZED)
    {
        Log(L"ERROR: MH_Initialize -> %d", (int)st);
        return false;
    }

    st = MH_CreateHook(target, (LPVOID)&DetourIl2cppInit, (LPVOID *)&g_realIl2cppInit);
    if (st != MH_OK)
    {
        Log(L"ERROR: MH_CreateHook -> %d", (int)st);
        return false;
    }

    st = MH_EnableHook(target);
    if (st != MH_OK)
    {
        Log(L"ERROR: MH_EnableHook -> %d", (int)st);
        return false;
    }

    Log(L"MinHook installed on il2cpp_init. trampoline=%p", (void *)g_realIl2cppInit);
    return true;
}
