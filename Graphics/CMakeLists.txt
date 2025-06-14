project(Graphics LANGUAGES CXX)

add_library(Graphics STATIC)

target_sources(Graphics
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Renderer.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Device.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Instance.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/PhysicalDevice.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/QueueFamily.cpp"
        "${CMAKE_SOURCE_DIR}/External/volk/volk.c"
)

target_include_directories(Graphics PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(Graphics PUBLIC ${CMAKE_SOURCE_DIR}/External)
target_include_directories(Graphics PUBLIC ${CMAKE_SOURCE_DIR}/External/VulkanSDK/Include)

target_compile_definitions(Graphics PRIVATE VK_USE_PLATFORM_WIN32_KHR)

target_link_libraries(Graphics PUBLIC Core)
target_compile_definitions(Graphics PRIVATE Graphics_EXPORTS)
add_dependencies(Graphics Core)