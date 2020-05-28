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


add_executable(relabel
        relabel-main.cpp
        relabel-common.cpp)

add_executable(relabel-unordered-map
        relabel-unordered-map.cpp
        relabel-common.cpp)
