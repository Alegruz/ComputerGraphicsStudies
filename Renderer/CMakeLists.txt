cmake_minimum_required (VERSION 3.8)

add_library(Renderer SHARED RendererMain.cpp "../External/volk/volk.c")
target_sources(Renderer PUBLIC FILE_SET CXX_MODULES FILES "Renderer.ixx")
target_include_directories(Renderer PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(Renderer PRIVATE Core)
target_include_directories(Renderer PUBLIC ${PROJECT_SOURCE_DIR}/External)
target_include_directories(Renderer PUBLIC ${PROJECT_SOURCE_DIR}/External/VulkanSDK/Include)
target_compile_definitions(Renderer PRIVATE VK_NO_PROTOTYPES)

if(UNIX)
    target_compile_definitions(Renderer PRIVATE VK_USE_PLATFORM_ANDROID_KHR CGS_UNIX)
endif()

if(WIN32)
    target_compile_definitions(Renderer PRIVATE CGS_WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif(APPLE)
    target_compile_definitions(Renderer PRIVATE CGS_APPLE)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)
elseif(ANDROID)
    target_compile_definitions(Renderer PRIVATE CGS_ANDROID)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_ANDROID_KHR)
elseif(UNIX)
    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
        target_compile_definitions(Renderer PRIVATE VK_USE_PLATFORM_ANDROID_KHR CGS_LINUX)
        if(DEFINED USE_XCB AND USE_XCB)
            find_package(XCB REQUIRED)
            target_include_directories(Renderer PRIVATE ${XCB_INCLUDE_DIRS})
            target_link_libraries(Renderer PRIVATE ${XCB_LIBRARIES})
            set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XCB_KHR)
        elseif(DEFINED USE_WAYLAND AND USE_WAYLAND)
            find_package(Wayland REQUIRED)
            target_include_directories(Renderer PRIVATE ${Wayland_INCLUDE_DIRS})
            target_link_libraries(Renderer PRIVATE ${Wayland_LIBRARIES})
            set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WAYLAND_KHR)
        else()
            find_package(X11 REQUIRED)
            target_include_directories(Renderer PRIVATE ${X11_INCLUDE_DIRS})
            target_link_libraries(Renderer PRIVATE ${X11_LIBRARIES})
            set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XLIB_KHR)
        endif()
    elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
        target_compile_definitions(Renderer PRIVATE CGS_FREE_BSD)
    endif()
else()
    message(WARNING "Unknown platform detected")
endif()

add_executable(RendererMain Main.cpp)
target_include_directories(RendererMain PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(RendererMain Renderer)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Renderer PROPERTY CXX_STANDARD 20)
  set_property(TARGET RendererMain PROPERTY CXX_STANDARD 20)
endif()

set_target_properties(Renderer PROPERTIES 
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin 
	LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib 
	ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

set_target_properties(RendererMain PROPERTIES 
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin 
	LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib 
	ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    target_compile_definitions(Renderer PRIVATE CGS_RELEASE)
    target_compile_definitions(RendererMain PRIVATE CGS_DEBUG)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    target_compile_definitions(Renderer PRIVATE CGS_RELEASE)
    target_compile_definitions(RendererMain PRIVATE CGS_RELEASE)
endif()