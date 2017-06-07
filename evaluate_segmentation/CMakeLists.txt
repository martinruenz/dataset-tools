cmake_minimum_required(VERSION 2.6.0)
project(evaluate_segmentation)

add_executable(evaluate_segmentation main.cpp ${SOURCE_FILES})
target_link_libraries(evaluate_segmentation ${LIBRARIES})
