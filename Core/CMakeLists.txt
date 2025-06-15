project(Core LANGUAGES CXX)

add_library(Core STATIC)

configure_cgs_target(Core)

target_sources(Core
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/pch.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Config.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Log.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/WindowWin32.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/WindowUnix.cpp"
)

target_include_directories(Core PUBLIC ${CMAKE_SOURCE_DIR})

target_compile_definitions(Core PRIVATE Core_EXPORTS)

target_precompile_headers(Core PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/pch.h")
