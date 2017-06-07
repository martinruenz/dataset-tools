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
extern const unsigned char L_COLORS[31][3];

/**
 * @brief Convert an id ('label') to a colour ('return')
 * @param label The id
 * @return Colour
 */
cv::Vec3b intToColour(int label);

/**
 * @brief Uses intToColor pixel-wise, to convert an id-image to an RGB image, using the colour-table above
 * @param input ID-image
 * @return Colour image
 */
cv::Mat labelToColourImage(cv::Mat input);

/**
 * @brief This function converts RGB-masks to id-masks
 * @param input Input image
 * @param colorTable Unique colours found in the input image. Can already be filled, in order to be consistent for all frames.
 * @param maxDiff Max difference between pixel-colour and colours in table, which still leads to an association.
 * @return id-image
 */
cv::Mat colourToLabelImage(cv::Mat input, std::vector<cv::Vec3b>& colorTable, float maxDiff = 0);
