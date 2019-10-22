cmake_minimum_required(VERSION 2.6.0)

project(extract_scnnet_sens)
add_executable(${PROJECT_NAME} extract.cpp ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

project(view_scnnet_sens)
add_executable(${PROJECT_NAME} viewer.cpp ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

project(test_scnnet)
add_executable(${PROJECT_NAME} test-sequences.cpp ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})
