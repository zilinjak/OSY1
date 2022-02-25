cmake_minimum_required(VERSION 3.10)
project(OSY1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -W -Wall -Wextra -pedantic")

add_library(progtest_solver SHARED IMPORTED)
set_target_properties(progtest_solver PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/x86_64-linux-gnu/libprogtest_solver.a)

add_executable(OSY1 common.h main.cpp progtest_solver.h sample_tester.cpp sample_tester.h)
target_link_libraries(OSY1 pthread libprogtest_solver.a)