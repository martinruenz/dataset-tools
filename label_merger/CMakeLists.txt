cmake_minimum_required(VERSION 2.6.0)
project(merge_labels)

add_executable(${PROJECT_NAME} main.cpp ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})
