cmake_minimum_required(VERSION 2.6.0)

project(LogView)

find_package(OpenCV REQUIRED)
find_package(Boost COMPONENTS thread system filesystem REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${ZLIB_INCLUDE_DIR})

add_definitions(-Wall)

set(CMAKE_CXX_FLAGS "-g -O3 -msse2 -msse3")

FILE(GLOB srcs *.cpp)
source_group("Source Files" FILES ${srcs} )

add_executable(LogView 
               ${srcs}
)

target_link_libraries(LogView
                      ${ZLIB_LIBRARY}
                      ${Boost_LIBRARIES}
                      ${OpenCV_LIBS})
