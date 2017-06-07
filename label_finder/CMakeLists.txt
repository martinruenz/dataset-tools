cmake_minimum_required(VERSION 2.6.0)
project(label_finder)

add_executable(label_finder main.cpp ${SOURCE_FILES} ../common/common_labels.cpp)
target_link_libraries(label_finder ${LIBRARIES})
