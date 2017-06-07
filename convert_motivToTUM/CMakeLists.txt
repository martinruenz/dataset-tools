cmake_minimum_required(VERSION 2.6.0)
project(convert_motivToTUM)

add_executable(convert_motivToTUM main.cpp ${SOURCE_FILES})
target_link_libraries(convert_motivToTUM ${LIBRARIES})
