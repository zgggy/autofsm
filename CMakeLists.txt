cmake_minimum_required(VERSION 3.10)
project(FiniteStateMachine)

set(CMAKE_CXX_COMPILER "c++")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SRC_LIST example/cat_example2.cpp)

add_executable(cat ${SRC_LIST})
