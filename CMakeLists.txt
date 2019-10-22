cmake_minimum_required(VERSION 3.5.0)
project(DatasetTools)

# TODO REMOVE
#if (Boost_NO_SYSTEM_PATHS)
#  set(BOOST_ROOT "/home/ruenz/source/boost_1_68_0")
#  set(BOOST_INCLUDE_DIRS "${BOOST_ROOT}/include")
#  set(BOOST_LIBRARY_DIRS "/home/ruenz/source/boost_1_68_0/stage/lib")
#endif (Boost_NO_SYSTEM_PATHS)

find_package( OpenCV 4.0.0 REQUIRED )
find_package( Eigen3 REQUIRED )
find_package( OpenMP )
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()
find_package( Boost 1.65.1 COMPONENTS filesystem system REQUIRED )

# c++ version
set(CMAKE_CXX_STANDARD 14)

# GCC warning / errors
if (CMAKE_COMPILER_IS_GNUCC)
  add_compile_options(-Werror -Wall -Wno-unused-function -Wno-unused-variable
    -Wno-unused-but-set-variable -Wno-unused-but-set-variable -Wno-write-strings
    -Wno-deprecated -Wno-deprecated-declarations -Werror -Wno-unknown-pragmas)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.0)
    add_compile_options(-Wno-misleading-indentation)
  endif()
endif()

set(LIBRARIES ${OpenCV_LIBRARIES} ${Boost_LIBRARIES})
include_directories(${EIGEN_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(convert_depth)
#add_subdirectory(convert_blenderMasks)
add_subdirectory(convert_masks)
add_subdirectory(convert_klg)
add_subdirectory(convert_imagesToKlg)
add_subdirectory(convert_exrToRgb)
add_subdirectory(convert_poses)
add_subdirectory(convert_motivToTUM)
add_subdirectory(convert_scannet_sens)
add_subdirectory(generate_trajectory)
add_subdirectory(label_merger)
add_subdirectory(label_associator)
add_subdirectory(label_finder)
add_subdirectory(merge_exr)
add_subdirectory(evaluate_segmentation)
#add_subdirectory(evaluate_reconstruction)
#add_subdirectory(evaluate_rgbd_camera)
add_subdirectory(object_coordinate_renderer_wip)
add_subdirectory(green_screener)

#add_subdirectory(trajectory_tool)

# Tell QtCreator to show these files
file(GLOB showfiles *.h *.md common/*.h common/*.cpp)
add_custom_target(showfiles SOURCES ${showfiles})
