cmake_minimum_required(VERSION 3.0)

project(ComputerGraphicsStudies)

set(CMAKE_WIN32_EXECUTABLE true)

add_executable(ComputerGraphicsStudies Win32Main.cpp)

if(CMAKE_WIN32_EXECUTABLE)
    target_compile_definitions(ComputerGraphicsStudies PRIVATE CGS_WINDOWS)
endif()