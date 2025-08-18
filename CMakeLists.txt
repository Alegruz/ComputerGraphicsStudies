cmake_minimum_required(VERSION 3.15)

project(ComputerGraphicsStudies LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 17)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(WIN32)
    set(CGS_WINDOWS TRUE)
    set(CGS_LINUX FALSE)
    set(CMAKE_WIN32_EXECUTABLE true)
    
    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
else()
    set(CGS_WINDOWS FALSE)
    set(CGS_LINUX TRUE)
endif()

add_library(project_warnings INTERFACE)

target_compile_options(project_warnings INTERFACE
    # MSVC
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive- /Zc:__cplusplus /EHsc /utf-8 /Zc:preprocessor /bigobj>
    # GCC/Clang
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
    -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
    -fno-exceptions -fno-rtti>  # if that’s your policy; otherwise drop
)

add_library(project_sanitizers INTERFACE)
target_compile_options(project_sanitizers INTERFACE $<$<AND:$<NOT:$<CXX_COMPILER_ID:MSVC>>,$<CONFIG:Debug>>:-fsanitize=address,undefined -fno-omit-frame-pointer>)
target_link_options(project_sanitizers INTERFACE $<$<AND:$<NOT:$<CXX_COMPILER_ID:MSVC>>,$<CONFIG:Debug>>:-fsanitize=address,undefined>)

if(CGS_WINDOWS)
    add_executable(ComputerGraphicsStudies Win32Main.cpp)
elseif(CGS_LINUX)
    add_executable(ComputerGraphicsStudies LinuxMain.cpp)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

set_target_properties(ComputerGraphicsStudies PROPERTIES OUTPUT_NAME "ComputerGraphicsStudies_${CMAKE_SYSTEM_NAME}_${CMAKE_CXX_COMPILER_ID}")

target_link_libraries(ComputerGraphicsStudies PRIVATE project_warnings)
if(ENABLE_SANITIZERS)
    target_link_libraries(ComputerGraphicsStudies PRIVATE project_sanitizers)
endif()

target_compile_features(ComputerGraphicsStudies PRIVATE cxx_constexpr cxx_lambda_init_captures)
if(CGS_WINDOWS)
    target_compile_definitions(ComputerGraphicsStudies PRIVATE CGS_WINDOWS)
elseif(CGS_LINUX)
    target_compile_definitions(ComputerGraphicsStudies PRIVATE CGS_LINUX)
endif()