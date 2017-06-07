cmake_minimum_required(VERSION 2.6.0)
project(convert_masks)

add_executable(convert_masks main.cpp ${SOURCE_FILES} ../common/common_labels.cpp)
target_link_libraries(convert_masks ${LIBRARIES})
