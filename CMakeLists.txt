cmake_minimum_required (VERSION 2.6)
project (WSN)
set (WSN_VERSION_MAJOR 1)
set (WSN_VERSION_MINOR 0)

SET(GCC_COVERAGE_COMPILE_FLAGS "-std=c++11 -pthread ")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")  
# add the executable

add_executable(wsn main.cpp)
target_link_libraries(wsn -lrt)