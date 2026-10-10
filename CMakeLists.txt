cmake_minimum_required(VERSION 3.15)
project(doorstep C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE DEPS_C "deps/*.c")

add_executable(doorstep WIN32)

target_sources(doorstep PRIVATE ${SOURCES} ${DEPS_C} "resources/common.rc")

target_include_directories(doorstep PRIVATE
    src
    deps
)

target_compile_definitions(doorstep PRIVATE
    NOMINMAX
    UNICODE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

target_link_libraries(doorstep PRIVATE
    shell32 kernel32 user32 advapi32 ntdll
)

target_compile_options(doorstep PRIVATE
    /W0
    /GR-
    /Od
    $<$<CONFIG:Debug>:/Zi>
)

target_link_options(doorstep PRIVATE
    $<$<CONFIG:Release>:/LTCG>
)