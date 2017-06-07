cmake_minimum_required(VERSION 2.6.0)
project(convert_klg)

find_package(ZLIB REQUIRED) #For compressed datasets
include_directories(${ZLIB_INCLUDE_DIR})

add_executable(convert_klg main.cpp ${SOURCE_FILES})
target_link_libraries(convert_klg -ljpeg ${LIBRARIES} ${ZLIB_LIBRARY})

# Tell QtCreator to show these files
file(GLOB showfiles_klg *.h *.hpp *.cpp)
add_custom_target(showfiles_klg SOURCES ${showfiles_klg})
