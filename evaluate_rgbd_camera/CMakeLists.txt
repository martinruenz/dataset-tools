cmake_minimum_required(VERSION 2.6.0)
project(evaluate_rgbd_camera)

add_executable(evaluate_rgbd_camera main.cpp ${SOURCE_FILES})
target_link_libraries(evaluate_rgbd_camera ${LIBRARIES})
