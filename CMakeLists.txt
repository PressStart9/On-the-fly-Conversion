cmake_minimum_required(VERSION 3.26.5)
project(lab6)

set(CMAKE_CXX_STANDARD 17)

add_executable(lab6 main.cpp)

target_compile_definitions(lab6 PRIVATE PRIVATE FUSE_USE_VERSION=29 _FILE_OFFSET_BITS=64)

find_library(
        FUSE_LIBRARIES
        NAMES fuse
        PATHS /lib64 /lib /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib /usr/lib/x86_64-linux-gnu
)

include(convertions/FetchLibraries.cmake)

target_link_libraries(lab6 PRIVATE fuse ${CONVERTION_LIBRARIES})
