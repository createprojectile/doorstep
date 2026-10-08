#include "config.h"
#include "coreclr_host.h"
#include "globals.h"
#include "il2cpp_hook.h"
#include "logger.h"
#include "path_utils.h"
#include <windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    g_exePath = GetExePathW();
    g_exeDir = GetDirName(g_exePath);

    LogInit(g_exeDir);
    Log(L"================ Doorstop Launcher ================");
    Log(L"exe      : %ls", g_exePath.c_str());
    Log(L"exe dir  : %ls", g_exeDir.c_str());

    SetCurrentDirectoryW(g_exeDir.c_str());

    g_targetAsmFull = ResolveAgainstExe(g_exeDir, cfg::kTargetAssembly);
    g_dotnetFull = ResolveAgainstExe(g_exeDir, cfg::kCorlibDir);
    g_bepInExCoreFull = ResolveAgainstExe(g_exeDir, cfg::kBepInExCore);

    Log(L"target   : %ls", g_targetAsmFull.c_str());
    Log(L"dotnet   : %ls", g_dotnetFull.c_str());
    Log(L"bepinex  : %ls", g_bepInExCoreFull.c_str());

    SetDoorstopEnvVars();

    std::wstring unityDll = PathJoin(g_exeDir, cfg::kUnityPlayerName);
    std::wstring gaDll = PathJoin(g_exeDir, cfg::kGameAssemblyName);

    Log(L"Loading UnityPlayer: %ls", unityDll.c_str());
    HMODULE hUnity = LoadLibraryW(unityDll.c_str());
    if (!hUnity)
    {
        Log(L"ERROR: LoadLibraryW(UnityPlayer) -> %lu", GetLastError());
        LogErrorBox(
            L"UnityPlayer.dll could not be loaded.\n\n"
            L"Make sure this EXE sits in the game's root folder,\n"
            L"next to the original UnityPlayer.dll."
        );
        return 1;
    }

    Log(L"Loading GameAssembly: %ls", gaDll.c_str());
    HMODULE hGameAsm = LoadLibraryW(gaDll.c_str());
    if (!hGameAsm)
    {
        Log(L"ERROR: LoadLibraryW(GameAssembly) -> %lu", GetLastError());
        LogErrorBox(
            L"GameAssembly.dll could not be loaded.\n\n"
            L"Is this really an IL2CPP Unity game?"
        );
        return 1;
    }

    if (!InstallIl2CppInitHook(hGameAsm))
    {
        Log(L"ERROR: could not install il2cpp_init hook.");
        LogErrorBox(
            L"Could not hook il2cpp_init via MinHook.\n\n"
            L"See doorstop.log for the MinHook status code."
        );
        return 1;
    }

    using UnityMainFn = int(WINAPI *)(HINSTANCE, HINSTANCE, LPWSTR, int);
    auto unityMain = (UnityMainFn)GetProcAddress(hUnity, cfg::kUnityMainExport);
    if (!unityMain)
    {
        Log(L"ERROR: export '%hs' not found in UnityPlayer.dll", cfg::kUnityMainExport);
        LogErrorBox(L"UnityPlayer.dll does not export UnityMain.");
        return 1;
    }

    Log(L"Calling UnityMain(...)");
    int rc = unityMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    Log(L"UnityMain returned %d", rc);
    Log(L"================ launcher exit ================");
    return rc;
}
