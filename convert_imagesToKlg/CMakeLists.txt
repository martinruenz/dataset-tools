cmake_minimum_required(VERSION 2.6.0)
project(convert_imagesToKlg)

add_executable(convert_imagesToKlg main.cpp ${SOURCE_FILES})
target_link_libraries(convert_imagesToKlg ${LIBRARIES})
