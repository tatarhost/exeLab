cmake_minimum_required(VERSION 3.24)
project(TokerExecutor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    Luau
    GIT_REPOSITORY https://github.com/luau-lang/luau.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Luau)

add_executable(Toker WIN32 src/main.cpp)
target_compile_definitions(Toker PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(Toker PRIVATE Luau.Compiler Luau.VM comctl32 comdlg32)
if(MSVC)
    target_compile_options(Toker PRIVATE /W4 /permissive- /EHsc)
    target_link_options(Toker PRIVATE /SUBSYSTEM:WINDOWS)
endif()
