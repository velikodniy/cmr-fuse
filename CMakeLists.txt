cmake_minimum_required(VERSION 4.0)
project(cmr_fuse VERSION 0.1)

add_executable(cmr_fuse
    cmr-fuse.c
    cmrapi.c
    list.c
    filelist_cache.c
    http_client.c
    utils.c)

include(cmake/CPM.cmake)
CPMAddPackage(
    URI "gh:afiskon/c-algorithms#71d02135871c37c2a653b690a42b15d846b1c441"
)
CPMAddPackage(
    URI "gh:akheron/jansson@2.14.1"
    OPTIONS "JANSSON_BUILD_DOCS OFF"
)
CPMAddPackage(
    URI "gh:macos-fuse-t/libfuse#2e7803a20eaa1cc0b136014033297392071191ac"
)

find_package(Threads REQUIRED)
find_package(CURL)

add_definitions(-D_FILE_OFFSET_BITS=64 -DPROJECT_VERSION=${PROJECT_VERSION})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -pedantic -Wall -Wextra")

target_include_directories(cmr_fuse
    PUBLIC "${jansson_BINARY_DIR}/include"
    PUBLIC "${c-algorithms_SOURCE_DIR}/include"
    PUBLIC "${libfuse_SOURCE_DIR}/include"
)
target_link_directories(cmr_fuse
    PUBLIC "${libfuse_BINARY_DIR}/lib"
)
target_link_libraries(cmr_fuse
    ${CMAKE_THREAD_LIBS_INIT}
    ${CURL_LIBRARIES}
    jansson
    CAlgorithms
    fuse-t
)
