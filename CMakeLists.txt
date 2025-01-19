# CMakeList.txt : CMake project for ComputerGraphicsStudies, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("ComputerGraphicsStudies")

set(PREBUILT_MODULE_DIR ${CMAKE_BINARY_DIR}/modules)

function(add_module name)
    file(MAKE_DIRECTORY ${PREBUILT_MODULE_DIR})
    add_custom_target(${name}.ifc
            COMMAND
                ${CMAKE_CXX_COMPILER}
                /std:c++latest
                /experimental:module
                /c
                /EHsc
                /MD
                ${CMAKE_CURRENT_SOURCE_DIR}/${ARGN}
                /module:export
                /ifcOutput
                ${PREBUILT_MODULE_DIR}/${name}.ifc
                /Fo${PREBUILT_MODULE_DIR}/${name}.obj
            )
endfunction()

add_subdirectory(Core)
add_subdirectory(Renderer)
add_subdirectory(External)

#if (CMAKE_VERSION VERSION_GREATER 3.12)
#  set_property(TARGET ComputerGraphicsStudies PROPERTY CXX_STANDARD 20)
#endif()

# TODO: Add tests and install targets if needed.
