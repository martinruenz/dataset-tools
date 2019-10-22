cmake_minimum_required(VERSION 2.6.0)
project(convert_klg)

find_package(ZLIB REQUIRED) #For compressed datasets
include_directories(${ZLIB_INCLUDE_DIR})

add_executable(${PROJECT_NAME} main.cpp ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} -ljpeg ${LIBRARIES} ${ZLIB_LIBRARY})
