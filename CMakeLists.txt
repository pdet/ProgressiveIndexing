cmake_minimum_required(VERSION 3.9)
project(ProgressiveIndexing)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE off)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MACOSX_RPATH 1)

set(CMAKE_C_FLAGS_DEFAULT "${CMAKE_CXX_FLAGS_DEFAULT}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")


set(SOURCES src/util/file_manager.cpp src/util/binary_search.cpp src/util/hybrid_radix_insert_sort.cpp
        src/progressive/progressive_quicksort.cpp src/progressive/progressive_bucketsort_equiheight.cpp
        src/progressive/progressive_radixsort_lsd.cpp src/progressive/progressive_radixsort_msd.cpp
        src/progressive/progressive_radixsort_msd_noquick.cpp src/progressive/progressive_radixsort_msd_noquick.cpp
        src/progressive/progressive_indexing.cpp)

add_library(PI_lib ${SOURCES})


add_executable(main src/main.cpp)
target_link_libraries(main PI_lib)

add_executable(generate_workload src/generate/generate_workload.cpp)
target_link_libraries(generate_workload PI_lib)

add_executable(generate_column src/generate/generate_column.cpp)
target_link_libraries(generate_column PI_lib)

add_executable(measure scripts/cost_model/measure.cpp)

add_subdirectory("test")