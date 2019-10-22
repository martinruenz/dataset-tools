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

#include "common_macros.h"

#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <iomanip>
#include <chrono>
#include <random>
#include <fstream>

#include <eigen3/Eigen/Dense>
#include <opencv2/imgproc/imgproc.hpp>
//void splitString(const std::string &s, char delim, std::vector<std::string> &elems);
//std::vector<std::string> splitString(const std::string &s, char delim, bool allowEmpty = true);

//// See: http://stackoverflow.com/questions/5891610/how-to-remove-characters-from-a-string
//void removeChars(std::string& str, char* charsToRemove);

//// Valid input example: "120,150,0"=>Vec3b(0,150,120) [or "55"=>Vec3b(55,55,55) iff allow_grayscale==true]
//std::tuple<uint8_t,uint8_t,uint8_t> stringToRgb(const std::string& color, bool allow_grayscale = false);
//cv::Vec3b stringToColor(const std::string& color, bool allow_grayscale = false);
//std::vector<cv::Vec3b> stringToColors(const std::string& colors, bool allow_grayscale = false);
//std::string colorToString(cv::Vec3b& color, bool is_id = false);
//bool isColorID(cv::Vec3b& color); // checks if all channels have same value

//Eigen::VectorXf stringToEigenVec(const std::string& color);

std::string ZeroPad(size_t value, int width){
    std::stringstream s;
    s << std::setfill('0') << std::setw(width) << value;
    return s.str();
}

// ---- String operations:
inline void splitString(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) elems.push_back(item);
    if (!ss && item.empty()) elems.push_back(""); // Check trailing (emtpy) item
}

inline std::vector<std::string> splitString(const std::string &s, char delim, bool allowEmpty = true) {
    std::vector<std::string> strings;
    splitString(s, delim, strings);
    if(!allowEmpty) strings.erase(std::remove(strings.begin(), strings.end(), ""), strings.end());
    return strings;
}

// See: http://stackoverflow.com/questions/5891610/how-to-remove-characters-from-a-string
inline void removeChars(std::string& str, char* charsToRemove) {
    for(unsigned int i = 0; i < strlen(charsToRemove); ++i)
        str.erase(remove(str.begin(), str.end(), charsToRemove[i]), str.end());
}

inline std::tuple<uint8_t, uint8_t, uint8_t> stringToRgb(const std::string &color, bool allow_grayscale = false){
    std::vector<std::string> rgbs = splitString(color, ',', false);
    std::vector<int> rgb;
    for(auto& c : rgbs) rgb.push_back(std::stoi(c));
    if(allow_grayscale && rgb.size() == 1) {
        rgb.push_back(rgb[0]);
        rgb.push_back(rgb[0]);
    }
    if(rgb.size() != 3) throw std::invalid_argument("Colour value not recognised.");
    for(int c : rgb) if(c < 0 || c > 255) throw std::invalid_argument("Colour value out of bounds.");
    return std::make_tuple<uint8_t, uint8_t, uint8_t>(rgb[0],rgb[1],rgb[2]);
}

inline cv::Vec3b stringToColor(const std::string& color, bool allow_grayscale = false){
    std::tuple<uint8_t, uint8_t, uint8_t> rgb = stringToRgb(color, allow_grayscale);
    return cv::Vec3b(std::get<2>(rgb),std::get<1>(rgb),std::get<0>(rgb)); //OpenCV is BGR
}

inline std::vector<cv::Vec3b> stringToColors(const std::string& colors, bool allow_grayscale = false){
    std::vector<std::string> rgbs = splitString(colors, ' ', false);
    std::vector<cv::Vec3b> result;
    for(auto& c : rgbs) result.push_back(stringToColor(c, allow_grayscale));
    return result;
}

inline bool isColorID(cv::Vec3b &color){
    return (color[0] == color[1]) && (color[1] == color[2]);
}

inline std::string colorToString(cv::Vec3b &color, bool is_id){
    if(is_id) {
        if(!isColorID(color)) throw std::runtime_error("Expected ID but got RGB color.");
        return std::to_string(color[0]);
    } else {
        return std::to_string(color[0])+","+std::to_string(color[1])+","+std::to_string(color[2]);
    }
}

inline Eigen::VectorXf stringToEigenVec(const std::string& color){
    std::vector<std::string> floats = splitString(color, ',', false);
    Eigen::VectorXf result(floats.size());
    for (size_t i = 0; i < floats.size(); ++i) result(i) = std::stof(floats[i]);
    return result;
}
