cmake_minimum_required(VERSION 3.6)
project(cmr_fuse)

# ------ LIBRARIES ------

# CURL

find_package(CURL)

# FUSE

if (APPLE)
    SET(FUSE_NAMES libosxfuse.dylib fuse)
else (APPLE)
    SET(FUSE_NAMES fuse)
endif (APPLE)

FIND_LIBRARY(FUSE_LIBRARIES
    NAMES ${FUSE_NAMES}
    PATHS /lib64 /lib /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib
)
include ("FindPackageHandleStandardArgs")
find_package_handle_standard_args ("FUSE" DEFAULT_MSG
        FUSE_INCLUDE_DIR FUSE_LIBRARIES)

mark_as_advanced (FUSE_INCLUDE_DIR FUSE_LIBRARIES)

# JANSSON

find_path(JANSSON_INCLUDE_DIR jansson.h)
find_library(JANSSON_LIBRARY NAMES jansson)

include_directories(SYSTEM ${JANSSON_INCLUDE_DIR})

# ------ END LIBRARIES ------

add_definitions(-D_FILE_OFFSET_BITS=64)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")

include_directories(containers/include)

add_executable(cmr_fuse
        cmr-fuse.c
        cmrapi.c
        list.c
        containers/lib/htable.c
        containers/lib/ilist.c
        containers/lib/rbtree.c
        )

target_link_libraries(cmr_fuse
        ${CURL_LIBRARIES}
        ${JANSSON_LIBRARY}
        ${FUSE_LIBRARIES}
        )