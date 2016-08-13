cmake_minimum_required(VERSION 3.5)
project(cmr_fuse)

# ------ LIBRARIES ------

# CURL

find_package(CURL)

# FUSE

FIND_LIBRARY(FUSE_LIBRARIES
    NAMES libosxfuse.dylib fuse
    PATHS /lib64 /lib /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib
)

# JANSSON

find_path(JANSSON_INCLUDE_DIR jansson.h)
find_library(JANSSON_LIBRARY NAMES jansson)

include_directories(SYSTEM ${JANSSON_INCLUDE_DIR})

# ------ END LIBRARIES ------

add_definitions(-D_FILE_OFFSET_BITS=64)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra")

include_directories(containers/include)
add_subdirectory(containers)

add_executable(cmr_fuse
        cmr-fuse.c
        cmrapi.c
        list.c
        filelist_cache.c)

target_link_libraries(cmr_fuse
        CAlgorithms
        ${CURL_LIBRARIES}
        ${JANSSON_LIBRARY}
        ${FUSE_LIBRARIES}
        )
