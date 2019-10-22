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

#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <iomanip>
#include <chrono>
#include <random>
#include <fstream>


#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

inline cv::Mat floatToUC1(cv::Mat image, float min=0, float max=255) {
    assert(max >= min);
    float range = max-min;
    cv::Mat tmp(image.rows, image.cols, CV_8UC1);

    auto convert = [&](float value) -> unsigned char {
        return (std::min(value, max) - min) / range * 255;
    };

    if(image.type() == CV_32FC1){
        for(int y = 0; y < image.rows; y++)
            for(int x = 0; x < image.cols; x++)
                tmp.at<unsigned char>(y,x) = convert(image.at<float>(y,x));
    } else {
        throw std::invalid_argument("Invalid EXR file / float image.");
    }
    return tmp;
}

inline void storeFloatImage(cv::Mat image, const std::string& path, float min=0, float max=255) {
    cv::imwrite(path, floatToUC1(image, min, max));
}

/**
 * @brief printComponentStatistics
 * @param stats Output of connectedComponentsWithStats
 */
inline void printComponentStatistics(cv::Mat_<int> stats){
    for(int c = 0; c < stats.rows; c++){
        std::cout << "Label: " << c
                  << ", size: " << stats.at<int>(c,4)
                  << ", box: [" << stats.at<int>(c,0)  << "," << stats.at<int>(c,1) << " " << stats.at<int>(c,2)  << "x" << stats.at<int>(c,3) << "]\n";
    }
}

inline std::string cvTypeToString(int type) {
    std::string r;
    uchar depth = type & CV_MAT_DEPTH_MASK;
    uchar chans = 1 + (type >> CV_CN_SHIFT);
    switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
    }
    r += "C";
    r += (chans+'0');
    return r;
}


inline cv::Mat imfill(cv::Mat image){
    assert(image.type() == CV_8UC1);
    cv::Mat result = image.clone();
    cv::floodFill(result, cv::Point(0,0), cv::Scalar(255));
    cv::bitwise_not(result, result);
    return (image | result);
}
