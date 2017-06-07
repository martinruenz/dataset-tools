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

#include "common_3d.h"

PinholeCamera::PinholeCamera(const PinholeParameters& params){
    assert(params.fx > 0);
    assert(params.fy > 0);
    this->params = params;
    inv_fx = 1.0 / params.fx;
    inv_fy = 1.0 / params.fy;
}
cv::Vec3d PinholeCamera::toRay(const cv::Vec2d& coord) const {
    return cv::normalize(cv::Vec3d(inv_fx * (coord[0] - params.cx), inv_fy * (coord[1] - params.cy), 1.0));
}
cv::Vec2d PinholeCamera::toPixel(const cv::Vec3d& coord) const {
    if(coord[2] <= 0) return cv::Vec2d(-1,-1);
    float inv_z = 1.0f / coord[2];
    return cv::Vec2d(params.fx * coord[0] * inv_z + params.cx, params.fy * coord[1] * inv_z + params.cy);
}


Pose::Pose(const Eigen::Vector3d& t_, const Eigen::Quaterniond& q_, double ts_) : t(t_), q(q_), ts(ts_){
}
Pose::Pose(const Eigen::Matrix4d& m, double ts_){
    t = m.topRightCorner(3, 1);
    Eigen::Matrix<double, 3, 3> rotObject = m.topLeftCorner(3, 3);
    q = Eigen::Quaterniond(rotObject);
    ts = ts_;
}
Eigen::Matrix4d Pose::toMatrix() const {
    Eigen::Matrix4d result = Eigen::Matrix4d::Identity();
    result.topLeftCorner(3, 3) = q.toRotationMatrix();
    result.topRightCorner(3, 1) = t;
    return result;
}

Projected3DCloud::Projected3DCloud(){}
Projected3DCloud::Projected3DCloud(const cv::Mat& depthMetric,
                 const cv::Mat& rgb,
                 const PinholeParameters& intrinsics,
                 float minDepth,
                 float maxDepth){
    fromMat(depthMetric, rgb, intrinsics, minDepth, maxDepth);
}

void Projected3DCloud::fromMat(const cv::Mat& depthMetric, const cv::Mat& rgb, const PinholeParameters& intrinsics, float minDepth, float maxDepth){
    this->minDepth = minDepth;
    this->maxDepth = maxDepth;
    height = depthMetric.rows;
    width = depthMetric.cols;

    assert(rgb.total() == 0 || rgb.total() == depthMetric.total());

    setSize(width, height);

    float fxInv = 1.0f / intrinsics.fx;
    float fyInv = 1.0f / intrinsics.fy;
    hasRGB = (rgb.total() != 0);

    for(size_t y = 0; y < height; y++){
        for(size_t x = 0; x < width; x++){

            Point3D point;

            // 3D
            point.p[2] = depthMetric.at<float>(y,x);
            point.p[0] = (x - intrinsics.cx) * point.p[2] * fxInv;
            point.p[1] = (y - intrinsics.cy) * point.p[2] * fyInv;

            // RGB
            if(hasRGB){
                cv::Vec3b color = rgb.at<cv::Vec3b>(y,x);
                point.rgb = cv::Vec3b(color[2],color[1],color[0]);
            }

            points[y][x] = point;
            if(point.p[2] >= minDepth && point.p[2] <= maxDepth) numValidPoints++;
        }
    }

    // Normals
    for(size_t y = 1; y < height-1; y++){
        for(size_t x = 1; x < width-1; x++){
            Point3D& point = points[y][x];
            cv::Vec3d dX = (points[y][x-1].p + point.p) * 0.5f - (points[y][x+1].p + point.p) * 0.5f;
            cv::Vec3d dY = (points[y-1][x].p + point.p) * 0.5f - (points[y+1][x].p + point.p) * 0.5f;
            point.n  = cv::normalize(-dX.cross(dY));
        }
    }

    // Normals at borders
    for(size_t y = 0; y < height; y++) {
        points[y][0].n = points[y][1].n;
        points[y][depthMetric.cols-1].n = points[y][depthMetric.cols-2].n;
    }
    for(size_t x = 1; x < width-1; x++){
        points[0][x].n = points[1][x].n;
        points[depthMetric.rows-1][x].n = points[depthMetric.rows-2][x].n;
    }
}

void Projected3DCloud::setSize(size_t width, size_t height){
    points.resize(height, std::vector<Point3D>(width));
}

Point3D& Projected3DCloud::at(size_t x, size_t y){
    return points[y][x];
}


void Projected3DCloud::toPly(const std::string& path){
    std::ofstream fileOut(path);
    fileOut << "ply\n";
    fileOut << "format ascii 1.0\n";
    fileOut << "comment object: Point cloud\n";
    fileOut << "element vertex " << numValidPoints << "\n";
    fileOut << "property float x\n";
    fileOut << "property float y\n";
    fileOut << "property float z\n";
    fileOut << "property float nx\n";
    fileOut << "property float ny\n";
    fileOut << "property float nz\n";
    if(hasRGB){
        fileOut << "property uchar red\n";
        fileOut << "property uchar green\n";
        fileOut << "property uchar blue\n";
    }
    fileOut << "end_header\n";
    for(size_t y = 0; y < height; y++){
        for(size_t x = 0; x < width; x++){
            Point3D& p = points[y][x];
            if(p.p[2] >= minDepth && p.p[2] <= maxDepth) {
                fileOut <<  p.p[0] << " " << p.p[1] << " " << p.p[2] << " " <<
                            p.n[0] << " " << p.n[1] << " " << p.n[2];

                if(hasRGB) fileOut << " " << (int)p.rgb[0] << " " << (int)p.rgb[1] << " " << (int)p.rgb[2];
                fileOut << "\n";
            }
        }
    }
    fileOut.close();
}
