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
#include "connected_labels.h"

// Example of distinct colours for labels
extern const unsigned char L_COLORS[31][3] = { {0,0,0},
                                      {0,0,255},
                                      {255,0,0},
                                      {0,255,0},
                                      {255,26,184},
                                      {255,211,0},
                                      {0,131,246},
                                      {0,140,70},
                                      {167,96,61},
                                      {79,0,105},
                                      {0,255,246},
                                      {61,123,140},
                                      {237,167,255},
                                      {211,255,149},
                                      {184,79,255},
                                      {228,26,87},
                                      {131,131,0},
                                      {0,255,149},
                                      {96,0,43},
                                      {246,131,17},
                                      {202,255,0},
                                      {43,61,0},
                                      {0,52,193},
                                      {255,202,131},
                                      {0,43,96},
                                      {158,114,140},
                                      {79,184,17},
                                      {158,193,255},
                                      {149,158,123},
                                      {255,123,175},
                                      {158,8,0}};

/**
 * @brief Convert an id ('label') to a colour ('return')
 * @param label The id
 * @return Colour
 */
cv::Vec3b intToColour(int label){
    return (label == 255) ? cv::Vec3b(255,255,255) : (cv::Vec3b)L_COLORS[label % 31];
}

/**
 * @brief Uses intToColor pixel-wise, to convert an id-image to an RGB image, using the colour-table above
 * @param input ID-image
 * @return Colour image
 */
cv::Mat labelToColourImage(cv::Mat input){
    cv::Mat result(input.rows, input.cols, CV_8UC3);
    cv::Vec3b* pixel_ptr = (cv::Vec3b*)result.data;
    if(input.type() == CV_8UC1) for(unsigned i = 0; i < result.total(); ++i) pixel_ptr[i] = intToColour(input.data[i]);
    else if(input.type() == CV_32SC1) for(unsigned i = 0; i < result.total(); ++i) pixel_ptr[i] = intToColour(((int*)input.data)[i]);
    else if(input.type() == CV_16UC1) for(unsigned i = 0; i < result.total(); ++i) pixel_ptr[i] = intToColour(((unsigned short*)input.data)[i]);
    else if(input.type() == CV_8UC3) for(unsigned i = 0; i < result.total(); ++i) pixel_ptr[i] = intToColour((((cv::Vec3b*)input.data)[i])[0]);
    else assert(0 && "labelToColourImage: Unknown input format.");
    return result;
}

/**
 * @brief This function converts RGB-masks to id-masks
 * @param input Input image
 * @param colorTable Unique colours found in the input image. Can already be filled, in order to be consistent for all frames.
 * @param maxDiff Max difference between pixel-colour and colours in table, which still leads to an association.
 * @return id-image
 */
cv::Mat colourToLabelImage(cv::Mat input, std::vector<cv::Vec3b>& colorTable, float maxDiff = 0){
    if(input.type() != CV_8UC3) { // This requirement could easily be avoided
        std::cout << "Error, wrong image format" << std::endl;
        return cv::Mat();
    }
    cv::Mat result(input.rows, input.cols, CV_8UC1);
    for (int i = 0; i < input.rows; ++i){
        //cv::Vec3b* pixel = input.ptr<cv::Vec3b>(i);
        unsigned char* pOut = result.ptr<unsigned char>(i);

        for (int j = 0; j < input.cols; ++j){
            cv::Vec3b pixel = input.at<cv::Vec3b>(i, j);
            int maskID = -1;
            for(size_t id = 0; id < colorTable.size(); id++){
                if(cv::norm(pixel,colorTable[id]) <= maxDiff) {
                    maskID = id;
                    break;
                }
            }
            if(maskID < 0) {
                colorTable.push_back(pixel);
                maskID = colorTable.size() - 1;
            }
            pOut[j] = maskID;
        }
    }
    return result;
}

/**
 * @brief Search for 'color' in 'colors' and return the index if found, -1 otherwise.
 * @param colors
 * @param color
 * @return
 */
int getColorIndex(const std::vector<cv::Vec3b>& colors, const cv::Vec3b& color){
    for(size_t i = 0; i < colors.size(); i++)
        if(color==colors[i])
            return i;
    return -1;
}

/**
 * @brief Find new colors in input and add them to 'colors'. Each color in 'colors' is supposed to be unique. Ignores background (0,0,0).
 * @param input Input image, has to be CV_8UC3.
 * @param colors Output vector of colors/labels. May already contain colors.
 */
void findColors(cv::Mat input, std::vector<cv::Vec3b>& colors){
    if(input.type() != CV_8UC3) {
        throw std::invalid_argument("Error, invalid image format.");
        return;
    }
//    for (int i = 0; i < input.rows; ++i){
//        for (int j = 0; j < input.cols; ++j){
//            Vec3b pixel = input.at<Vec3b>(i, j);
//            int colorIndex = getColorIndex(colors, pixel);
//            if(colorIndex < 0 && pixel != Vec3b(0,0,0)) colors.push_back(pixel);
//        }
//    }
    for (size_t i = 0; i < input.total(); ++i){
        cv::Vec3b pixel = input.at<cv::Vec3b>(i);
        int colorIndex = getColorIndex(colors, pixel);
        if(colorIndex < 0 && pixel != cv::Vec3b(0,0,0)) colors.push_back(pixel);
    }
}
