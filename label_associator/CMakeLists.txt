cmake_minimum_required(VERSION 2.6.0)
project(label_associator)

add_executable(label_associator main.cpp ${SOURCE_FILES} ../common/common_labels.cpp ../common/connected_labels.cpp)
target_link_libraries(label_associator ${LIBRARIES})
