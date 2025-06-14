project(Engine LANGUAGES CXX)

add_library(Engine STATIC)

target_sources(Engine
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/pch.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Engine.cpp"
)

target_include_directories(Engine PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(Engine PUBLIC ${CMAKE_SOURCE_DIR}/External)

target_compile_definitions(Engine PRIVATE Core_EXPORTS)

target_precompile_headers(Engine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/pch.h")
target_link_libraries(Engine PRIVATE Graphics)
