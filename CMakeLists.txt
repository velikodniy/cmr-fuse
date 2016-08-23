cmake_minimum_required(VERSION 3.5)
project(cmr_fuse VERSION 0.1)

# ------ LIBRARIES ------

# pthreads

find_package(Threads REQUIRED)

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

add_definitions(-D_FILE_OFFSET_BITS=64 -DPROJECT_VERSION=${PROJECT_VERSION})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -pedantic -Wall -Wextra")

include_directories(containers/include)
add_subdirectory(containers)

add_executable(cmr_fuse
        cmr-fuse.c
        cmrapi.c
        list.c
        filelist_cache.c
        http_client.c
        utils.c)

target_link_libraries(cmr_fuse
        ${CMAKE_THREAD_LIBS_INIT}
        CAlgorithms
        ${CURL_LIBRARIES}
        ${JANSSON_LIBRARY}
        ${FUSE_LIBRARIES}
        )
