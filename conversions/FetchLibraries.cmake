include(FetchContent)

set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG f056911
)

FetchContent_MakeAvailable(stb)

add_library(
    stb_lib INTERFACE
)
target_include_directories(stb_lib INTERFACE ${stb_SOURCE_DIR})

set(
    conversion_LIBRARIES
    stb_lib
)
