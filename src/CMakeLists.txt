include_directories(include)

add_subdirectory(experiment)
add_subdirectory(util)
add_subdirectory(progressive)
add_subdirectory(index)
add_subdirectory(measurements)
add_subdirectory(sort)


add_library(ProgressiveIndexing experiment/experiments.cpp )
target_link_libraries(ProgressiveIndexing experiments util progressive index sort cost_model)

add_executable(PIMain main.cpp)
target_link_libraries(PIMain ProgressiveIndexing)
