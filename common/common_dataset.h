/******************************************************************
This file is part of https://github.com/martinruenz/dataset-tools

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*****************************************************************/

#pragma once

#include <vector>
#include <sophus/se3.hpp>

#include "common_filesystem.h"

template <typename T>
struct DatasetInterface
{
    virtual size_t countCameras() = 0;
    virtual size_t countObjects() = 0;
    virtual size_t countFrames() = 0;
    virtual Sophus::SE3<T> getTworld_object(size_t frame_index, size_t object_id = 0) = 0;
    virtual Sophus::SE3<T> getTworld_camera(size_t frame_index, size_t camera_id = 0) = 0;
    virtual Sophus::SE3<T> getTcamera_object(size_t frame_index, size_t camera_id = 0, size_t object_id = 0) = 0;
    //virtual std::unique_ptr<GlMesh> getMesh(size_t object_id = 0) = 0;
};


template <typename derived>
void GetPosesFromRbotTxt(const std::string& file_path, std::vector<Sophus::SE3<derived>>& out_poses)
{
    if(!exists(file_path)){
        throw std::invalid_argument("File does not exist: " + file_path);
    }

    std::string line;
    std::ifstream file(file_path);

    Eigen::Matrix<derived, 3, 3> rotation;
    Eigen::Matrix<derived, 3, 1> translation;
    while(std::getline(file, line))
    {
        if(!isdigit(line[0]) && line[0] != '-')
            continue;
        std::istringstream iss(line);
        iss >> rotation(0,0) >> rotation(0,1) >> rotation(0,2) >> rotation(1,0) >> rotation(1,1) >> rotation(1,2) >> rotation(2,0) >> rotation(2,1) >> rotation(2,2) >> translation(0) >> translation(1) >> translation(2);
        out_poses.push_back(Sophus::SE3<derived>(Eigen::Quaternion<derived>(rotation), translation));
    }
}
