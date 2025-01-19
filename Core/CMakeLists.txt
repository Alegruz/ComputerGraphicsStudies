cmake_minimum_required (VERSION 3.8)

add_library(Core STATIC)
target_sources(Core PUBLIC FILE_SET CXX_MODULES FILES "Math-Vector.ixx" "Math-Matrix.ixx" "Image.ixx")
target_include_directories(Core PUBLIC ${PROJECT_SOURCE_DIR})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Core PROPERTY CXX_STANDARD 20)
endif()

set_target_properties(Core PROPERTIES 
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin 
	LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib 
	ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    target_compile_definitions(Core PRIVATE CGS_DEBUG)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    target_compile_definitions(Core PRIVATE CGS_RELEASE)
endif()