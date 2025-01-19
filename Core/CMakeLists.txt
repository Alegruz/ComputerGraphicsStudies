cmake_minimum_required (VERSION 3.8)

add_library(Core)
target_sources(Core PUBLIC FILE_SET CXX_MODULES FILES   "Math-Vector.ixx" "Math-Matrix.ixx"  "Image.ixx")
target_include_directories(Core PUBLIC ${PROJECT_SOURCE_DIR})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Core PROPERTY CXX_STANDARD 20)
endif()