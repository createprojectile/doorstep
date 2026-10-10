#pragma once

namespace cfg
{
    constexpr wchar_t kTargetAssembly[] = L"RustClient_Data\\BepInEx\\core\\BepInEx.Unity.IL2CPP.dll";
    constexpr char kTargetAssemblyName[] = "BepInEx.Unity.IL2CPP";
    constexpr char kEntryType[] = "Doorstop.Entrypoint";
    constexpr char kEntryMethod[] = "Start";
    constexpr wchar_t kCorlibDir[] = L"RustClient_Data\\BepInEx\\dotnet";
    constexpr wchar_t kBepInExCore[] = L"RustClient_Data\\BepInEx\\core";
    constexpr wchar_t kUnityPlayerName[] = L"UnityPlayer.dll";
    constexpr wchar_t kGameAssemblyName[] = L"GameAssembly.dll";
    constexpr char kUnityMainExport[] = "UnityMain";
    constexpr char kIl2CppInitExport[] = "il2cpp_init";
    constexpr bool kEnableLogging = false;
    constexpr bool kContinueOnManagedFailure = true;
} // namespace cfg
