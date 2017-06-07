cmake_minimum_required(VERSION 2.6.0)
project(DatasetTools)

find_package( OpenCV REQUIRED )
find_package( Eigen3 REQUIRED )
include_directories(${EIGEN_INCLUDE_DIRS})

#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "-Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-write-strings -Wno-deprecated -Wno-deprecated-declarations -Werror -std=c++14")
set(LIBRARIES ${OpenCV_LIBRARIES} boost_filesystem boost_system)

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

SET(SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/common/common.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/common_3d.cpp"
)

add_subdirectory(convert_depth)
add_subdirectory(convert_masks)
add_subdirectory(convert_klg)
add_subdirectory(convert_imagesToKlg)
add_subdirectory(convert_exrToRgb)
add_subdirectory(convert_poses)
add_subdirectory(convert_motivToTUM)
add_subdirectory(label_merger)
add_subdirectory(label_associator)
add_subdirectory(label_finder)
add_subdirectory(evaluate_segmentation)
add_subdirectory(evaluate_reconstruction)
add_subdirectory(evaluate_rgbd_camera)

add_subdirectory(trajectory_tool)

# Tell QtCreator to show these files
file(GLOB showfiles *.h *.md common/*.h common/*.cpp)
add_custom_target(showfiles SOURCES ${showfiles})

