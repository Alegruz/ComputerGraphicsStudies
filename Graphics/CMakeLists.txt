project(Graphics LANGUAGES CXX)

add_library(Graphics STATIC)

configure_cgs_target(Graphics)

target_sources(Graphics
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/pch.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Renderable.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Renderer.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RendererManager.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RenderGraph.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Attachment.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/CommandBuffer.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/CommandPool.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Device.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Fence.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Image.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Instance.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Pipeline.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/PhysicalDevice.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/PhysicalDeviceGroup.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Queue.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/QueueFamily.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Resource.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Semaphore.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/SwapChain.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RHI/VertexLayout.cpp"
)

target_include_directories(Graphics PUBLIC ${CMAKE_SOURCE_DIR})
target_include_directories(Graphics PUBLIC ${CMAKE_SOURCE_DIR}/External)
target_include_directories(Graphics PUBLIC ${CMAKE_SOURCE_DIR}/External/VulkanSDK/Include)

target_link_libraries(Graphics PRIVATE pugixml gfx slang slang_rt)
target_link_libraries(Graphics PUBLIC Core volk rapidobj::rapidobj)
target_compile_definitions(Graphics PRIVATE Graphics_EXPORTS)
add_dependencies(Graphics Core)

target_precompile_headers(Graphics PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/pch.h")

add_custom_command(TARGET Graphics POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/External/slang/bin/slang-rt.dll
    $<TARGET_FILE_DIR:Graphics>
)

add_custom_command(TARGET Graphics POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/External/slang/bin/slang.dll
    $<TARGET_FILE_DIR:Graphics>
)

add_custom_command(TARGET Graphics POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/External/slang/bin/gfx.dll
    $<TARGET_FILE_DIR:Graphics>
)

add_custom_command(TARGET Graphics POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/External/slang/bin/slang-llvm.dll
    $<TARGET_FILE_DIR:Graphics>
)