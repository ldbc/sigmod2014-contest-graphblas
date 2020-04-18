cmake_minimum_required(VERSION 3.15)
project(sigmod2014pc_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# https://github.com/ben-strasser/fast-cpp-csv-parser/pull/93
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

link_libraries(Threads::Threads)

link_libraries(graphblas lagraph)

add_executable(sigmod2014pc_cpp
        main.cpp
        load.cpp
        utils.cpp)

add_executable(grb-example
        grb-example.cpp)

add_executable(q1
        q1.cpp
        load.cpp
        utils.cpp)
add_executable(q3
        q3.cpp
        load.cpp
        utils.cpp)
add_executable(q4
        q4.cpp
        load.cpp
        utils.cpp)
