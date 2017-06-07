cmake_minimum_required(VERSION 2.6.0)
project(merge_labels)

add_executable(merge_labels main.cpp ${SOURCE_FILES})
target_link_libraries(merge_labels ${LIBRARIES})
