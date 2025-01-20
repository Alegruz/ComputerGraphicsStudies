cmake_minimum_required (VERSION 3.8)

add_executable(RendererMain)
target_sources(RendererMain PUBLIC
    ${PROJECT_SOURCE_DIR}/External/volk/volk.c

    ${PROJECT_SOURCE_DIR}/Core/Math-Vector.ixx
    ${PROJECT_SOURCE_DIR}/Core/Math-Matrix.ixx
    ${PROJECT_SOURCE_DIR}/Core/Image.ixx
    ${PROJECT_SOURCE_DIR}/Core/Window.ixx

    ${PROJECT_SOURCE_DIR}/Renderer/Renderer.ixx

    ${PROJECT_SOURCE_DIR}/Application/Main.cpp
)
target_include_directories(RendererMain PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(RendererMain PUBLIC ${PROJECT_SOURCE_DIR}/External)
target_include_directories(RendererMain PUBLIC ${PROJECT_SOURCE_DIR}/External/VulkanSDK/Include)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET RendererMain PROPERTY CXX_STANDARD 20)
endif()

if(UNIX)
    target_compile_definitions(RendererMain PRIVATE CGS_UNIX)
endif()

if(WIN32)
    target_compile_definitions(RendererMain PRIVATE VK_USE_PLATFORM_WIN32_KHR CGS_WIN32)
elseif(APPLE)
    target_compile_definitions(RendererMain PRIVATE VK_USE_PLATFORM_MACOS_MVK CGS_APPLE)
elseif(ANDROID)
    target_compile_definitions(RendererMain PRIVATE VK_USE_PLATFORM_ANDROID_KHR CGS_ANDROID)
elseif(UNIX)
    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
        target_compile_definitions(RendererMain PRIVATE VK_USE_PLATFORM_ANDROID_KHR CGS_LINUX)
        if(DEFINED USE_XCB AND USE_XCB)
            find_package(XCB REQUIRED)
            target_include_directories(RendererMain PRIVATE ${XCB_INCLUDE_DIRS})
            target_link_libraries(RendererMain PRIVATE ${XCB_LIBRARIES})
        target_compile_definitions(RendererMain PRIVATE VK_USE_PLATFORM_XCB_KHR)
        elseif(DEFINED USE_WAYLAND AND USE_WAYLAND)
            find_package(Wayland REQUIRED)
            target_include_directories(RendererMain PRIVATE ${Wayland_INCLUDE_DIRS})
            target_link_libraries(RendererMain PRIVATE ${Wayland_LIBRARIES})
        target_compile_definitions(RendererMain PRIVATE VK_USE_PLATFORM_WAYLAND_KHR)
        else()
            find_package(X11 REQUIRED)
            target_include_directories(RendererMain PRIVATE ${X11_INCLUDE_DIRS})
            target_link_libraries(RendererMain PRIVATE ${X11_LIBRARIES})
        target_compile_definitions(RendererMain PRIVATE VK_USE_PLATFORM_XLIB_KHR)
        endif()
    elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
        target_compile_definitions(RendererMain PRIVATE CGS_FREE_BSD)
    endif()
else()
    message(WARNING "Unknown platform detected")
endif()

message(STATUS hiiiiiiiiiiii)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    target_compile_definitions(RendererMain PRIVATE CGS_DEBUG)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    target_compile_definitions(RendererMain PRIVATE CGS_RELEASE)
endif()