cmake_minimum_required(VERSION 3.15)
project(sigmod2014pc_cpp)

set(CMAKE_CXX_STANDARD 17)

link_libraries(graphblas lagraph)

add_executable(sigmod2014pc_cpp main.cpp)
