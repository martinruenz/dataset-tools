cmake_minimum_required(VERSION 2.6.0)
project(convert_depth)

add_executable(convert_depth main.cpp ${SOURCE_FILES})
target_link_libraries(convert_depth ${LIBRARIES})
