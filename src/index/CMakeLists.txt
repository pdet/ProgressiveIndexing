include_directories(../include)

set(index_source bptree.cpp index.cpp)

add_library(index ${index_source})
