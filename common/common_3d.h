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

    PinholeCamera(const PinholeParameters& params){
        assert(params.fx > 0);
        assert(params.fy > 0);
        this->params = params;
        inv_fx = 1.0 / params.fx;
        inv_fy = 1.0 / params.fy;
    }

    Eigen::Vector3d toRay(const Eigen::Vector2d& coord) const {
        return Eigen::Vector3d(inv_fx * (coord[0] - params.cx), inv_fy * (coord[1] - params.cy), 1.0).normalized();
    }
    Eigen::Vector2d toPixel(const Eigen::Vector3d& coord) const {
        if(coord.z() <= 0) return Eigen::Vector2d(-1,-1);
        float inv_z = 1.0f / coord[2];
        return Eigen::Vector2d(params.fx * coord[0] * inv_z + params.cx, params.fy * coord[1] * inv_z + params.cy);
    }
};

struct Point3D {
    cv::Vec3b rgb; //in this order
    Eigen::Vector3d p; //position
    Eigen::Vector3d n; //normal
};

struct Pose {
    Pose(const Eigen::Vector3d& t, const Eigen::Quaterniond& q, double ts=-1) : t(t), q(q), ts(ts) {}
    Pose(const Eigen::Matrix4d& m, double ts_=-1){
        t = m.topRightCorner(3, 1);
        Eigen::Matrix<double, 3, 3> rotObject = m.topLeftCorner(3, 3);
        q = Eigen::Quaterniond(rotObject);
        ts = ts_;
    }
    Eigen::Matrix4d toMatrix() const {
        Eigen::Matrix4d result = Eigen::Matrix4d::Identity();
        result.topLeftCorner(3, 3) = q.toRotationMatrix();
        result.topRightCorner(3, 1) = t;
        return result;
    }
    Eigen::Vector3d t;
    Eigen::Quaterniond q;
    double ts;
};

struct Projected3DCloud {

    Projected3DCloud(){}
    Projected3DCloud(const cv::Mat& depthMetric,
                     const cv::Mat& rgb,
                     const PinholeParameters& intrinsics,
                     float minDepth = 1e-7,
                     float maxDepth = std::numeric_limits<float>::max()){
        fromMat(depthMetric, rgb, intrinsics, minDepth, maxDepth);
    }

    Projected3DCloud(const std::vector<Point3D>& cloud,
                     const PinholeParameters& intrinsics,
                     size_t height, size_t width,
                     float minDepth = 1e-7,
                     float maxDepth = std::numeric_limits<float>::max()){
        fromCloud(cloud, intrinsics, height, width, minDepth, maxDepth);
    }

    void fromMat(const cv::Mat& depthMetric,
                 const cv::Mat& rgb,
                 const PinholeParameters& intrinsics,
                 float minDepth = 1e-7,
                 float maxDepth = std::numeric_limits<float>::max()){
        CHECK_THROW((depthMetric.cols > 0) && (depthMetric.rows > 0));
        setDepth(minDepth, maxDepth);
        setSize(depthMetric.cols, depthMetric.rows);
        assert(rgb.total() == 0 || rgb.total() == depthMetric.total());
        assert(depthMetric.type() == CV_32FC1);


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
                if(isValid(point)) numValidPoints++;
            }
        }

        // Normals
        for(size_t y = 1; y < height-1; y++){
            for(size_t x = 1; x < width-1; x++){
                Point3D& point = points[y][x];
                Eigen::Vector3d dX = (points[y][x-1].p + point.p) * 0.5f - (points[y][x+1].p + point.p) * 0.5f;
                Eigen::Vector3d dY = (points[y-1][x].p + point.p) * 0.5f - (points[y+1][x].p + point.p) * 0.5f;
                point.n  = -dX.cross(dY).normalized();
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

    void fromCloud(const std::vector<Point3D> &cloud,
                                     const PinholeParameters& intrinsics,
                                     size_t height,
                                     size_t width,
                                     float minDepth = 1e-7,
                                     float maxDepth = std::numeric_limits<float>::max()){
        setDepth(minDepth, maxDepth);
        setSize(width, height,true);
        PinholeCamera cam(intrinsics);
        for(const Point3D& p : cloud){
            Eigen::Vector2d pixel = cam.toPixel(p.p);
            if(isInside(pixel)){
                const Point3D& ref = points[pixel[1]][pixel[0]];
                if(isValid(p)) {
                    if(!isValid(ref)){
                        points[pixel[1]][pixel[0]] = p;
                        numValidPoints++;
                    } else if(p.p[2] < ref.p[2]){
                        points[pixel[1]][pixel[0]] = p;
                    }
                }
            }
        }
    }

    void setSize(size_t width, size_t height, bool initialize = false){
        this->width = width;
        this->height = height;
        points.resize(height, std::vector<Point3D>(width));
        if(initialize){
            #pragma omp parallel for
            for(size_t y = 0; y < height; y++)
                for(size_t x = 0; x < width; x++)
                    points[y][x].p = Eigen::Vector3d(0,0,0);
        }
    }

    void setDepth(float minDepth, float maxDepth){
        this->minDepth = minDepth;
        this->maxDepth = maxDepth;
        assert(minDepth < maxDepth);
    }

    Point3D& at(size_t x, size_t y){
        return points[y][x];
    }

    std::vector<Point3D> applyTransformation(const Eigen::Matrix4d& transformation){
        std::vector<Point3D> result;
        result.resize(numValidPoints);
        Eigen::Matrix4d transformation_n = transformation.inverse().transpose();
        size_t i = 0;

        auto apply_to_vec3 = [](const Eigen::Matrix4d& T, const Eigen::Vector3d& v) -> Eigen::Vector3d {
            //cv::Vec4d tmp = T * cv::Mat(cv::Vec4d(v[0],v[1],v[2],1));
            // T * Eigen::Vector4d(v(0),v(1),v(2),1);
            Eigen::Vector4d tmp = T * Eigen::Vector4d(v(0),v(1),v(2),1);
            return Eigen::Vector3d(tmp(0),tmp(1),tmp(2));
        };

        #pragma omp parallel for
        for(size_t y = 0; y < height; y++){
            for(size_t x = 0; x < width; x++){
                Point3D& p = points[y][x];
                if(isValid(p)){
                    Point3D out;
                    out.rgb = p.rgb;
                    out.p = apply_to_vec3(transformation, p.p);
                    out.n = apply_to_vec3(transformation_n, p.n);
                    result[i++] = out;
                }
            }
        }
        assert(numValidPoints == i);
        return result;
    }

    void toPly(const std::string& path){
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
                if(isValid(p)) {
                    fileOut <<  p.p[0] << " " << p.p[1] << " " << p.p[2] << " " <<
                                p.n[0] << " " << p.n[1] << " " << p.n[2];

                    if(hasRGB) fileOut << " " << (int)p.rgb[0] << " " << (int)p.rgb[1] << " " << (int)p.rgb[2];
                    fileOut << "\n";
                }
            }
        }
        fileOut.close();
    }

    cv::Mat getMask() const{
        cv::Mat result = cv::Mat::zeros(height, width, CV_8UC1);
        for(size_t y = 0; y < height; y++){
            uchar* r = result.ptr<uchar>(y);
            for(size_t x = 0; x < width; x++){
                if(isValid(points[y][x])) r[x] = 255;
            }
        }
        return result;
    }

    bool isInside(const Eigen::Vector2d& p) const{
        return p[0] >= 0 && p[1] >= 0 && p[0] < width && p[1] < height;
    }
    bool isValid(const Point3D &point) const{
        return (point.p[2] >= minDepth && point.p[2] <= maxDepth);
    }

private:
    size_t height, width;
    size_t numValidPoints = 0;
    float minDepth;
    float maxDepth;
    bool hasRGB;
    std::vector<std::vector<Point3D> > points;
};
