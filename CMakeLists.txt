cmake_minimum_required(VERSION 3.15)

project(ComputerGraphicsStudies LANGUAGES C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 17)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ENABLE_PCH "Enable precompiled headers" ON)

find_package(CUDAToolkit REQUIRED)

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

    # Wayland + pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
    pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)

    # Where the protocol XML files live (xdg-shell.xml, etc.)
    pkg_get_variable(WAYLAND_PROTOCOLS_DATADIR wayland-protocols pkgdatadir)
    if(NOT WAYLAND_PROTOCOLS_DATADIR)
        message(FATAL_ERROR "wayland-protocols not found via pkg-config")
    endif()

    # wayland-scanner (code generator)
    find_program(WAYLAND_SCANNER NAMES wayland-scanner REQUIRED)

    set(XDG_XML "${WAYLAND_PROTOCOLS_DATADIR}/stable/xdg-shell/xdg-shell.xml")
    set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/wayland-protocols")

    add_custom_command(
        OUTPUT  "${GEN_DIR}/xdg-shell-client-protocol.h" "${GEN_DIR}/xdg-shell-protocol.c"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
        COMMAND ${WAYLAND_SCANNER} client-header "${XDG_XML}" "${GEN_DIR}/xdg-shell-client-protocol.h"
        COMMAND ${WAYLAND_SCANNER} private-code "${XDG_XML}" "${GEN_DIR}/xdg-shell-protocol.c"
        DEPENDS "${XDG_XML}"
        VERBATIM
    )

    add_library(xdg_shell OBJECT "${GEN_DIR}/xdg-shell-protocol.c")
    target_include_directories(xdg_shell PUBLIC "${GEN_DIR}")

    # Prefer -pthread over -lpthread when available
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
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
    add_executable(ComputerGraphicsStudies 
        Win32Main.cpp 
        )
elseif(CGS_LINUX)
    add_executable(ComputerGraphicsStudies 
        LinuxMain.cpp
        $<TARGET_OBJECTS:xdg_shell>
        )
    target_include_directories(ComputerGraphicsStudies PRIVATE ${WAYLAND_INCLUDE_DIRS})
    target_link_libraries(ComputerGraphicsStudies PRIVATE ${WAYLAND_LIBRARIES})
    target_link_libraries(ComputerGraphicsStudies PRIVATE PkgConfig::WAYLAND_CLIENT)
    target_include_directories(ComputerGraphicsStudies PRIVATE "${GEN_DIR}")
    target_link_libraries(ComputerGraphicsStudies PRIVATE PkgConfig::CAIRO)
    target_link_libraries(ComputerGraphicsStudies PRIVATE Threads::Threads)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

target_include_directories(ComputerGraphicsStudies PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

if(ENABLE_PCH)
    target_precompile_headers(ComputerGraphicsStudies PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/pch.hpp>"
    )
endif()

if(MSVC)
    target_compile_definitions(ComputerGraphicsStudies PRIVATE CGS_COMPILER_MSVC)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_definitions(ComputerGraphicsStudies PRIVATE CGS_COMPILER_CLANG)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(ComputerGraphicsStudies PRIVATE CGS_COMPILER_GCC)
else()
    message(FATAL_ERROR "Unsupported compiler")
endif()

if(DEFINED GRAPHICS_API_TYPE)
    message(STATUS "Using render device ${GRAPHICS_API_TYPE}")
    if(GRAPHICS_API_TYPE MATCHES "CPU")
        target_compile_definitions(ComputerGraphicsStudies PRIVATE CGS_GRAPHICS_API_CPU)
    elseif(GRAPHICS_API_TYPE MATCHES "D3D12")
        target_compile_definitions(ComputerGraphicsStudies PRIVATE CGS_GRAPHICS_API_D3D12)
        target_link_libraries(ComputerGraphicsStudies PRIVATE d3d12 dxgi)
    else()
        message(FATAL_ERROR "Unsupported graphics API type")
    endif()
endif()

set_target_properties(ComputerGraphicsStudies PROPERTIES OUTPUT_NAME "ComputerGraphicsStudies_${CMAKE_SYSTEM_NAME}_${CMAKE_CXX_COMPILER_ID}_${CMAKE_BUILD_TYPE}")

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

add_custom_command(TARGET ComputerGraphicsStudies POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:ComputerGraphicsStudies>" "${CMAKE_CURRENT_LIST_DIR}/${CMAKE_PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}"
)