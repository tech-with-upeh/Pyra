cmake_minimum_required(VERSION 3.16)
project(Helios LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(helios
    main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/semantics.cpp
    src/webengine/webengine.cpp
)

target_include_directories(helios
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/web
)

# Optional: warnings (safe)
if (MSVC)
    target_compile_options(helios PRIVATE /w)  #W4
else()
    target_compile_options(helios PRIVATE -w)
endif()
