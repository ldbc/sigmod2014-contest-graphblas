cmake_minimum_required(VERSION 3.10)
project(sigmod2014pc_cpp)

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "7.0.0")
        message(FATAL_ERROR "Insufficient gcc version")
    endif ()
endif ()

# https://github.com/ben-strasser/fast-cpp-csv-parser/pull/93
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

link_libraries(graphblas lagraph)

add_subdirectory(libcuckoo)

find_package(OpenMP REQUIRED)
link_libraries(OpenMP::OpenMP_CXX)

add_executable(sigmod2014pc_cpp
        main.cpp
        load.cpp
        utils.cpp
        query-parameters.cpp)

add_executable(paramgen
        paramgen-main.cpp
        load.cpp
        utils.cpp)

set(NNODES_MILLION_VALUE 100)
if (NNODES_MILLION)
        set(NNODES_MILLION_VALUE ${NNODES_MILLION})
endif()

set(NEDGES_MILLION_VALUE 200)
if (NEDGES_MILLION)
        set(NEDGES_MILLION_VALUE ${NEDGES_MILLION})
endif()

add_library(compile-time-definitions INTERFACE)
target_compile_definitions(compile-time-definitions INTERFACE NNODES_MILLION=${NNODES_MILLION_VALUE} NEDGES_MILLION=${NEDGES_MILLION_VALUE})

add_library(link-flags INTERFACE)
target_link_libraries(link-flags INTERFACE -static-libgcc -static-libstdc++)

add_executable(generate-relabel-data
        generate-relabel-data.cpp
        relabel-common.cpp)
target_link_libraries(generate-relabel-data PRIVATE compile-time-definitions link-flags)

add_executable(relabel
        relabel-main.cpp
        relabel-common.cpp)
target_link_libraries(relabel PRIVATE compile-time-definitions link-flags)

add_executable(relabel-libcuckoo
        relabel-libcuckoo.cpp
        relabel-common.cpp)
target_link_libraries(relabel-libcuckoo PRIVATE compile-time-definitions link-flags)

add_executable(relabel-unordered-map
        relabel-unordered-map.cpp
        relabel-common.cpp)
target_link_libraries(relabel-unordered-map PRIVATE compile-time-definitions link-flags)

add_executable(relabel-unordered-map-single-thread
        relabel-unordered-map.cpp
        relabel-common.cpp)
target_compile_definitions(relabel-unordered-map-single-thread PRIVATE NUMBER_OF_VERTEX_MAPPER_THREADS=1)
target_link_libraries(relabel-unordered-map-single-thread PRIVATE compile-time-definitions link-flags)

option(BUILD_CONCURRENT "Build the concurrent hash map based relabeling test" OFF)

# To build these use -DBUILD_CONCURRENT=ON -DTBB_DIR=<path-to-tbb>/cmake
if (BUILD_CONCURRENT)
        find_package(TBB REQUIRED)

        add_executable(relabel-concurrent-hash-map
                relabel-concurrent-hash-map.cpp
                relabel-common.cpp)
        target_link_libraries(relabel-concurrent-hash-map PRIVATE TBB::tbb)
        target_link_libraries(relabel-concurrent-hash-map PRIVATE compile-time-definitions link-flags)

        add_executable(relabel-concurrent-hash-map-single-thread
                relabel-concurrent-hash-map.cpp
                relabel-common.cpp)
        target_link_libraries(relabel-concurrent-hash-map-single-thread PRIVATE TBB::tbb)
        target_compile_definitions(relabel-concurrent-hash-map-single-thread PRIVATE NUMBER_OF_VERTEX_MAPPER_THREADS=1)
        target_link_libraries(relabel-concurrent-hash-map-single-thread PRIVATE compile-time-definitions link-flags)

        add_executable(relabel-concurrent-unordered-map
                relabel-concurrent-unordered-map.cpp
                relabel-common.cpp)
        target_link_libraries(relabel-concurrent-unordered-map PRIVATE TBB::tbb)
        target_link_libraries(relabel-concurrent-unordered-map PRIVATE compile-time-definitions link-flags)

        add_executable(relabel-concurrent-unordered-map-single-thread
                relabel-concurrent-unordered-map.cpp
                relabel-common.cpp)
        target_link_libraries(relabel-concurrent-unordered-map-single-thread PRIVATE TBB::tbb)
        target_compile_definitions(relabel-concurrent-unordered-map-single-thread PRIVATE NUMBER_OF_VERTEX_MAPPER_THREADS=1)
        target_link_libraries(relabel-concurrent-unordered-map-single-thread PRIVATE compile-time-definitions link-flags)

        add_executable(relabel-growt
                relabel-growt.cpp
                relabel-common.cpp)
        target_compile_options(relabel-growt PRIVATE -mcx16)
        target_link_libraries(relabel-growt PRIVATE TBB::tbb)
        target_link_libraries(relabel-growt PRIVATE compile-time-definitions link-flags)
        target_include_directories(relabel-growt PRIVATE growt)
endif()
