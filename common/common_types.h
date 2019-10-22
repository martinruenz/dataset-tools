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
#include <eigen3/Eigen/Dense>

// Like boost optional or std::experimental::optional
template<typename T>
struct FailableResult{
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    FailableResult(T d) : data(d), valid(true) {}
    FailableResult() : valid(false) {}
    T data;
    bool valid;
};
