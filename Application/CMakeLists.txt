file(GLOB MAIN_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)
add_executable(RendererMain ${MAIN_SOURCES})
target_include_directories(RendererMain PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(RendererMain PRIVATE Graphics)