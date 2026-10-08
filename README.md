# Doorstep

Замена [UnityDoorstop](https://github.com/NeighTools/UnityDoorstop) для IL2CPP-игр на Unity.
Никаких proxy-DLL (`winhttp.dll`, `version.dll`, `dxgi.dll`), никаких конфиг-файлов на диске.

## Зачем

UnityDoorstop работает через подмену системной библиотеки: рядом с игровым `.exe`
кладётся `winhttp.dll`, и загрузчик подхватывает её раньше системной.
Это работает, но у подхода две проблемы:

**1. Две `winhttp.dll` в одном процессе.** Proxy-DLL загружается первой по порядку поиска,
а настоящая системная `winhttp.dll` нужна самому Doorstop, чтобы пробросить вызовы дальше —
ит он подгружает её уже из `System32`. В итоге в процессе живут **две копии одного модуля**
с разными базами, разными глобалами и разными состояниями сессий. Экспорты настоящей DLL
прокси резолвит динамически через `GetProcAddress`, так что ручные заглушки не нужны, но
сам список имён лежит в `proxylist.txt` и генерируется кодогенератором на этапе сборки —
то есть это замороженный снимок на момент коммита. Экспорты, добавленные в Windows
позже, в него не попадают, и игра, ожидающая такой экспорт, получит `GetProcAddress == NULL`.

**2. Конфиг-файл.** Doorstep читает настройки из `doorstop_config.ini`, который обязан
лежать рядом с DLL. В production-сборке это значит лишний файл в дистрибутиве,
который игрок может случайно удалить, изменить или прочитать — там видны пути
к managed-сборкам и точка входа.

Doorstep решает обе проблемы: это **самостоятельный лаунчер-EXE**, который делает
всё сам, и вся конфигурация **зашита в бинарник на этапе компиляции**.

## Как это работает

```
Doorstep.exe (переименован под игру)
├── читает пути из констант компиляции        src/config.h
├── ставит переменные окружения DOORSTOP_*     src/coreclr_host.cpp
├── LoadLibraryW("UnityPlayer.dll")            src/main.cpp
├── LoadLibraryW("GameAssembly.dll")
├── MinHook перехватывает il2cpp_init          src/il2cpp_hook.cpp
│   └── после реального il2cpp_init:
│       ├── coreclr_initialize(APP_PATHS=...)  src/coreclr_host.cpp
│       ├── coreclr_create_delegate(BepInEx.Unity.IL2CPP, Doorstep.Entrypoint, Start)
│       └── вызов делегата → BepInEx загружен
└── вызов UnityMain(...) — возвращает управление игре
```

Ключевой момент: bootstrap отложен до момента вызова `il2cpp_init`. К этому моменту
загружены и рантайм IL2CPP, и `GameAssembly.dll`, поэтому BepInEx стартует с корректным
окружением, а не в пустом процессе до Unity.

Если CoreCLR не поднялся, лаунчер **не падает**: игрок всё равно получает рабочую игру
(см. `kContinueOnManagedFailure` в `config.h`). Ошибка пишется в лог и показывается
в `MessageBox`.

## Отличия от UnityDoorstop

| | UnityDoorstop | Doorstep |
|---|---|---|
| Метод внедрения | proxy-DLL, подхватывается по порядку поиска | отдельный EXE-лаунчер |
| Модулей в процессе | две копии `winhttp.dll` (прокси + системная) | ни одной подменённой DLL |
| Файлы в папке игры | `winhttp.dll`, `doorstop_config.ini` | только `Doorstep.exe` |
| Конфигурация | INI-файл на диске | `constexpr` в `src/config.h` |
| Хук | IAT-патчинг `LoadLibrary` | MinHook на `il2cpp_init` |
| Хостинг .NET | `coreclr_initialize` | `coreclr_initialize` |
| Архитектура | x86 / x64 | x64 |

## Требования

- Windows x64
- IL2CPP-сборка Unity (`GameAssembly.dll` + `UnityPlayer.dll` в корне игры)
- Билд-тайм: Visual Studio 2022 (MSVC), CMake 3.15+, C++23
- Рантайм-каталог `BepInEx` в папке данных игры: `dotnet/coreclr.dll` и `core/BepInEx.Unity.IL2CPP.dll`

## Сборка

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

На выходе — `build/Release/doorstep.exe`.

MinHook уже лежит в `deps/minhook` — дополнительных шагов не требуется.

Статический CRT (`MultiThreaded`), чтобы не тянуть за собой `vcruntime140.dll` рядом с игрой.

## Конфигурация

Всё в `src/config.h`, правится и пересобирается:

```cpp
constexpr wchar_t kTargetAssembly[] = L"RustClient_Data\\BepInEx\\core\\BepInEx.Unity.IL2CPP.dll";
constexpr char     kEntryType[]      = "Doorstop.Entrypoint";
constexpr char     kEntryMethod[]    = "Start";
constexpr wchar_t  kCorlibDir[]      = L"RustClient_Data\\BepInEx\\dotnet";
constexpr wchar_t  kUnityPlayerName[]= L"UnityPlayer.dll";
constexpr wchar_t  kGameAssemblyName[]= L"GameAssembly.dll";
constexpr char     kUnityMainExport[]= "UnityMain";
constexpr bool     kEnableLogging = true;
constexpr bool     kContinueOnManagedFailure = true;
```

Относительные пути разрешаются от папки EXE. Абсолютные — используются как есть.

Для другого BepInEx-плагина достаточно поменять `kEntryType` / `kEntryMethod`.
Это же даёт главное production-свойство: пути и точка входа **не видны игроку** и не
ломаются, если он удалит или подменит файлы в папке.

## Установка

1. Переименовать `doorstep.exe` в имя EXE игры.
2. Переименовать оригинальный EXE игры (например, в `Game.exe.bak`), чтобы лаунчер стал точкой входа.
3. Разложить содержимое BepInEx-папки по путям из `config.h`.
4. Убедиться, что `UnityPlayer.dll` и `GameAssembly.dll` лежат в корне рядом с лаунчером.

## Логи

`doorstop.log` в папке EXE, UTF-8, по записи на событие. Отключается `kEnableLogging = false`.
Ценой отладки в проде: при `false` у игрока не будет ни лога, ни диагностики,
поэтому для production-сборок рекомендуется оставлять логирование включённым
и чистить файл при развёртывании.

## Структура проекта

```
src/
├── main.cpp            точка входа: загрузка Unity, установка хука, вызов UnityMain
├── config.h            вся конфигурация на constexpr
├── il2cpp_hook.cpp     MinHook-перехват il2cpp_init
├── coreclr_host.cpp    coreclr_initialize + coreclr_create_delegate, переменные DOORSTOP_*
├── path_utils.cpp      пути, UTF-8 конвертация, проверка существования
├── logger.cpp          doorstop.log + диалоги ошибок
└── globals.cpp         глобалы exeDir / целевые пути
deps/minhook/           MinHook, включён в репозиторий (Tsuda Kageyu, BSD-2-Clause)
```

## Ограничения

- Только x64 и только IL2CPP. Mono-сборки не поддерживаются.
- Требуется экспорт `UnityMain` в `UnityPlayer.dll` и `il2cpp_init` в `GameAssembly.dll`.
- Лаунчер занимает имя игрового EXE, поэтому оригинал нужно переименовать
  (см. «Установка»). Процесс запускается под другим путём и именем, чем ожидает
  игра: проверьте целевой продукт перед релизом.
- Запуск происходит до инициализации IL2CPP, поэтому подгрузка нативных плагинов
  должна идти через BepInEx, а не напрямую через `LoadLibrary` из лаунчера.

## Лицензия

MIT — см. [LICENSE](LICENSE).

## Сторонние компоненты

**MinHook** — уже включён в репозиторий, лежит в `deps/minhook`.
Автор: **Tsuda Kageyu**, <https://github.com/TsudaKageyu/minhook>
Лицензия: BSD-2-Clause (`deps/minhook`, либо `LICENSE.txt` в исходниках MinHook).