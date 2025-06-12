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

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# Apply CGS_WIN32 to all targets
add_compile_definitions(CGS_WIN32)

# Apply build-type-specific definitions to all targets
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(CGS_DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_definitions(CGS_RELEASE)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

add_subdirectory(Core)
add_subdirectory(Graphics)
add_subdirectory(Application)