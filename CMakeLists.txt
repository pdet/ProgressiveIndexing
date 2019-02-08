cmake_minimum_required(VERSION 3.9)
project(ProgressiveIndexing)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE off)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MACOSX_RPATH 1)

set(CMAKE_CXX_FLAGS_DEFAULT "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -O3")

set(CMAKE_C_FLAGS_DEFAULT "${CMAKE_CXX_FLAGS_DEFAULT}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

if (NOT CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "Choose the type of build by setting -DCMAKE_BUILD_TYPE=<type>, options are: Debug Release RelWithDebInfo MinSizeRel.")
endif()

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

add_executable(generate_column
        src/util/file_manager.cpp src/generate/generate_column.cpp)

add_executable(generate_workload
        src/util/file_manager.cpp src/generate/generate_workload.cpp)

add_executable(main
        src/util/file_manager.cpp src/util/binary_search.cpp src/full_index/hybrid_radix_insert_sort.cpp
        src/full_index/bulkloading_bp_tree.cpp src/util/avl_tree.cpp src/cracking/cracking_util.cpp
        src/cracking/standard_cracking.cpp src/cracking/stochastic_cracking.cpp
        src/cracking/progressive_stochastic_cracking.cpp
        src/progressive/progressive_quicksort.cpp
        src/cracking/cracking_updates.cpp src/main.cpp)

add_executable(measure
        scripts/cost_model/measure.cpp)