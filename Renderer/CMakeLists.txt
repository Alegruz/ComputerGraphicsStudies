cmake_minimum_required (VERSION 3.8)

add_library(Renderer SHARED RendererMain.cpp)
target_include_directories(Renderer PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(Renderer PRIVATE Core)
target_compile_definitions(Renderer PRIVATE RENDERER_EXPORTS)

add_executable(RendererMain Main.cpp)
target_include_directories(RendererMain PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(RendererMain Renderer)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Renderer PROPERTY CXX_STANDARD 20)
  set_property(TARGET RendererMain PROPERTY CXX_STANDARD 20)
endif()