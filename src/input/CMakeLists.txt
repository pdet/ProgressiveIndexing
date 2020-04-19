include_directories(../include)

set(input_source file_manager.cpp)

set(generate_column_source file_manager.cpp generate_column.cpp)

#set(generate_query_source file_manager.cpp generate_query.cpp)

add_library(input ${input_source})

add_executable(generate_column
        ${generate_column_source})

#add_executable(generate_query
#        ${generate_query_source})
