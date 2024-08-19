cmake_minimum_required(VERSION 3.28)
project(FiniteStateMachine)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-std=c++17 -fconcepts)

set(SRC_LIST example/cat.cc)

add_executable(cat ${SRC_LIST})
