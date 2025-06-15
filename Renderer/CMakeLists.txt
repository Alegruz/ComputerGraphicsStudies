file(GLOB MAIN_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_executable(RendererApp ${MAIN_SOURCES})
configure_cgs_target(RendererApp)
target_include_directories(RendererApp PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(RendererApp PRIVATE Engine)