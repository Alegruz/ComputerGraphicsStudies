file(GLOB MAIN_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_executable(LauncherApp ${MAIN_SOURCES})
configure_cgs_target(LauncherApp)
target_include_directories(LauncherApp PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(LauncherApp PRIVATE ${PROJECT_SOURCE_DIR}/External/fltk)
target_link_libraries(LauncherApp PRIVATE Engine fltk)
target_precompile_headers(LauncherApp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/pch.h")