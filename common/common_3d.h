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

#include "common.h"

struct PinholeParameters {
    double cx, cy;
    double fx, fy;
};
struct PinholeCamera {

    PinholeParameters params;
    double inv_fx;
    double inv_fy;

    PinholeCamera(const PinholeParameters& params);
    cv::Vec3d toRay(const cv::Vec2d& coord) const;
    cv::Vec2d toPixel(const cv::Vec3d& coord) const;
};

struct Point3D {
    cv::Vec3b rgb; //in this order
    cv::Vec3d p; //position
    cv::Vec3d n; //normal
};

struct Pose {
    Pose(const Eigen::Vector3d& t, const Eigen::Quaterniond& q, double ts = -1);
    Pose(const Eigen::Matrix4d& m, double ts = -1);
    Eigen::Matrix4d toMatrix() const;
    Eigen::Vector3d t;
    Eigen::Quaterniond q;
    double ts;
};

struct Projected3DCloud {

    Projected3DCloud();
    Projected3DCloud(const cv::Mat& depthMetric,
                     const cv::Mat& rgb,
                     const PinholeParameters& intrinsics,
                     float minDepth = 0,
                     float maxDepth = std::numeric_limits<float>::max());

    void fromMat(const cv::Mat& depthMetric, const cv::Mat& rgb, const PinholeParameters& intrinsics, float minDepth = 0, float maxDepth = std::numeric_limits<float>::max());

    void setSize(size_t width, size_t height);
    Point3D& at(size_t x, size_t y);

    void toPly(const std::string& path);

private:
    size_t height, width;
    size_t numValidPoints = 0;
    float minDepth;
    float maxDepth;
    bool hasRGB;
    std::vector<std::vector<Point3D> > points;
};
