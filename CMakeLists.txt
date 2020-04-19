cmake_minimum_required(VERSION 3.10)

SET(CMAKE_CXX_STANDARD 14)

project(ProgressiveIndexing)

include_directories(src/include/)
add_subdirectory(src)
add_subdirectory(test)
