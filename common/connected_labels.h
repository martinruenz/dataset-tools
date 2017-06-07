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

#include <opencv2/imgproc/imgproc.hpp>
#include <map>
#include <list>

struct ComponentData {
    unsigned char label;
    int top = std::numeric_limits<int>::max();
    int right = 0;
    int bottom = 0;
    int left = std::numeric_limits<int>::max();
    float centerX = 0;
    float centerY = 0;
    int size = 0;
};

// This should not be faster than the next one, which is easier to use.
//void mapLabelsToComponents(const std::vector<ComponentData>& ccStats, std::map<int, std::list<int>>& labelToComponents){
//    assert(labelToComponents.find(ccStats[i].label) != labelToComponents.end());
//    for(unsigned i=0; i < ccStats.size(); i++) labelToComponents[ccStats[i].label].push_back(i);
//}

std::map<int, std::list<int>> mapLabelsToComponents(const std::vector<ComponentData>& ccStats);

cv::Mat connectedLabels(cv::Mat input, std::vector<ComponentData>* stats);
