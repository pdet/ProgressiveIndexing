include_directories(../include)

set(progressive_source qs_avl_tree.cpp progressive_index.cpp progressive_quicksort.cpp)

add_library(progressive ${progressive_source})

target_link_libraries(progressive util)
