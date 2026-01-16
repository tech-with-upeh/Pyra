cmake_minimum_required(VERSION 3.16)
project(helios LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(helios
    src/devserver.cpp
)

if(WIN32)
    find_package(Boost REQUIRED COMPONENTS beast)
        target_link_libraries(helios PRIVATE
            ws2_32
            mswsock
            Boost::beast
        )
endif() 

target_include_directories(helios
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include/devx
)

# Optional: warnings (safe)
if (MSVC)   
    target_compile_options(helios PRIVATE /w)  #W4
else()
    target_compile_options(helios PRIVATE -w)
endif()
