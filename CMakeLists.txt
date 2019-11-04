cmake_minimum_required(VERSION 3.9)
project(ProgressiveIndexing)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE off)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MACOSX_RPATH 1)

set(CMAKE_C_FLAGS_DEFAULT "${CMAKE_CXX_FLAGS_DEFAULT}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11   -msse -msse2 -msse3 -msse4.1 -msse4.2 -mavx ") #-g -O0

if (NOT CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "Choose the type of build by setting -DCMAKE_BUILD_TYPE=<type>, options are: Debug Release RelWithDebInfo MinSizeRel.")
endif ()

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif ()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

add_executable(generate_column
        src/util/file_manager.cpp src/generate/generate_column.cpp)


add_executable(generate_workload
        src/util/file_manager.cpp src/util/binary_search.cpp src/generate/generate_workload.cpp)

add_executable(main
        src/util/file_manager.cpp src/util/binary_search.cpp src/util/hybrid_radix_insert_sort.cpp
        src/progressive/progressive_quicksort.cpp src/progressive/progressive_bucketsort_equiheight.cpp
        src/progressive/progressive_radixsort_lsd.cpp src/progressive/progressive_radixsort_msd.cpp
        src/progressive/progressive_radixsort_msd_noquick.cpp  src/main.cpp)

add_executable(measure
        scripts/cost_model/measure.cpp)
