#set(micro_source alloc.cpp)
include_directories(../include)
set(micro_source cost_model.cpp sort_cost.cpp)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

#find_package(jemalloc)

#include_directories(${JEMALLOC_INCLUDE_DIR})

#add_executable(measurements ${micro_source})

#add_executable(match match.cpp)

add_library(cost_model cost_model.cpp)

add_executable(sort_cost ../sort/sort.cpp sort_cost.cpp)
