cmake_minimum_required(VERSION 3.26.5)
project(on-the-fly_conversion)

set(CMAKE_CXX_STANDARD 17)

add_executable(on-the-fly_conversion main.cpp conversions/stb_conversions.cpp)

target_compile_definitions(on-the-fly_conversion PRIVATE FUSE_USE_VERSION=29 _FILE_OFFSET_BITS=64)

find_library(
        FUSE_LIBRARIES
        NAMES fuse
        PATHS /lib64 /lib /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib /usr/lib/x86_64-linux-gnu
)

include(conversions/FetchLibraries.cmake)

target_link_libraries(on-the-fly_conversion PRIVATE fuse ${conversion_LIBRARIES})
