cmake_minimum_required(VERSION 3.15)
project(helios)

set(CMAKE_CXX_STANDARD 20)

# Collect all page sources
file(GLOB_RECURSE PAGES_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/pages/*.cpp")

# Add executable (this is just a dummy target for CMake)
add_executable(helios
    src/generated.cpp
    src/vdom.cpp
    ${PAGES_SRC}
)

# Include dirs
target_include_directories(helios
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

set(CMAKE_EXECUTABLE_SUFFIX ".js")

# Set default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Compiler flags based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(EMCC_FLAGS "-O0 -g0 -sASSERTIONS=1 -sALLOW_MEMORY_GROWTH")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(EMCC_FLAGS "-O3 -sASSERTIONS=0 -sALLOW_MEMORY_GROWTH")
endif()

# Apply flags to target
if(EMSCRIPTEN)
    set_target_properties(helios PROPERTIES
        COMPILE_FLAGS "${EMCC_FLAGS}"
        LINK_FLAGS "${EMCC_FLAGS} -sEXPORTED_FUNCTIONS=['_main','_invokeVNodeCallback','_js_insertHTML','_js_setTitle','_malloc','_free','_handleRoute','_animatefps','_handleEvent'] -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','stringToUTF8','lengthBytesUTF8']"
    )
    set_target_properties(helios PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/dist"
    OUTPUT_NAME "main"
    )
endif()

