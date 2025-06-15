# CMakeList.txt : CMake project for ComputerGraphicsStudies, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.12...3.27)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("ComputerGraphicsStudies")

# Platform-specific Vulkan defines
if(WIN32)
  add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
elseif(UNIX)
  add_compile_definitions(VK_USE_PLATFORM_XLIB_KHR)
endif()

include_directories(${CMAKE_SOURCE_DIR}/External/VulkanSDK/Include)

set(VULKAN_HEADERS_INSTALL_DIR "${CMAKE_SOURCE_DIR}/External/VulkanSDK/Include")

add_subdirectory(External/volk)
add_subdirectory(External/fltk)

foreach(t fltk fltk_images fltk_gl fltk-options fluid fluid-lib)
  if(TARGET ${t})
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      target_compile_options(${t} PRIVATE /wd4100 /wd4201 /wd4324 /WX-)
    else()
      target_compile_options(${t} PRIVATE -Wno-error -Wno-unused-parameter -Wno-missing-field-initializers)
    endif()
  endif()
endforeach()

# Platform-specific Vulkan defines
if(WIN32)
  message(STATUS "Building on Windows")
elseif(UNIX)
  message(STATUS "Building on Unix-like system")
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  message(STATUS "Compiler: MSVC")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(STATUS "Compiler: Clang")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message(STATUS "Compiler: GCC")
else()
  message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()  

function(configure_cgs_target target)
  if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_target_properties(${target} PROPERTIES
      CXX_STANDARD 20
      CXX_STANDARD_REQUIRED ON
      CXX_EXTENSIONS OFF
    )
  endif()

  if(WIN32)
    target_compile_definitions(${target} PRIVATE CGS_WIN32)
  elseif(UNIX)
    target_compile_definitions(${target} PRIVATE CGS_UNIX)
  endif()

  if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(${target} PRIVATE CGS_COMPILER_MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX /EHsc /wd4201)
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_definitions(${target} PRIVATE CGS_COMPILER_CLANG)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Werror -pedantic -Wno-missing-field-initializers)
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(${target} PRIVATE CGS_COMPILER_GCC)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Werror -pedantic -Wno-missing-field-initializers)
  endif()

  # Apply build-type-specific definitions to the target
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${target} PRIVATE CGS_DEBUG)
  elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${target} PRIVATE CGS_RELEASE)
  endif()

  # Set output directories for the target
  set_target_properties(${target} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
  )
endfunction()

add_subdirectory(Core)
add_subdirectory(Graphics)
add_subdirectory(Engine)
add_subdirectory(Launcher)
add_subdirectory(Renderer)
