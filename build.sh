#!/bin/bash -e

# Function that executes the clone command given as $1 iff repo does not exist yet. Otherwise pulls.
# Only works if repository path ends with '.git'
# Example: git_clone "git clone --branch 3.4.1 --depth=1 https://github.com/opencv/opencv.git"
function git_clone(){
  repo_dir=`basename "$1" .git`
  git -C "$repo_dir" pull 2> /dev/null || eval "$1"
}

# Ensure that current directory is root of project
cd $(dirname `realpath $0`)

# Enable colors
source external/bash_colors/bash_colors.sh
function highlight(){
  clr_magentab clr_bold clr_white "$1"
}

cd external
highlight "Building opencv..."
git_clone "git clone --depth=1 https://github.com/opencv/opencv.git"
cd opencv
mkdir -p build
cd build
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="`pwd`/../install" \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_JAVA=OFF \
  -DBUILD_PERF_TESTS=OFF \
  -DBUILD_PROTOBUF=OFF \
  -DBUILD_TESTS=OFF \
  -DBUILD_opencv_apps=OFF \
  -DBUILD_opencv_calib3d=OFF \
  -DBUILD_opencv_dnn=OFF \
  -DBUILD_opencv_gapi=OFF \
  -DBUILD_opencv_java=OFF \
  -DBUILD_opencv_java_bindings_generator=OFF \
  -DBUILD_opencv_ml=OFF \
  -DBUILD_opencv_objdetect=OFF \
  -DBUILD_opencv_python2=OFF \
  -DBUILD_opencv_python3=OFF \
  -DBUILD_opencv_python_bindings_generator=OFF \
  -DBUILD_opencv_python_tests=OFF \
  -DBUILD_opencv_stitching=OFF \
  -DWITH_1394=OFF \
  -DWITH_PROTOBUF=OFF \
  -DWITH_VTK=OFF \
  ..
make -j8
cd ../../..

mkdir build
cd build
cmake \
  -DOpenCV_DIR="$(pwd)/../external/opencv/build" \
  ..
make -j8
