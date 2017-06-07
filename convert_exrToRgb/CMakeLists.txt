cmake_minimum_required(VERSION 2.6.0)
project(convert_exrToRgb)

add_executable(convert_exrToRgb main.cpp ${SOURCE_FILES})
target_link_libraries(convert_exrToRgb ${LIBRARIES})
