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

#include <eigen3/Eigen/Dense>


// ---- Noise generation
struct NoiseGenerator{

    NoiseGenerator();
    virtual float generate() = 0;
    std::default_random_engine generator;
};

struct GaussianNoise : NoiseGenerator {
    GaussianNoise(float mean, float stddev);
    virtual float generate();
    std::normal_distribution<float> distribution;
};

struct CauchyNoise : NoiseGenerator {
    CauchyNoise(float peak, float scale);
    virtual float generate();
    std::cauchy_distribution<float> distribution;
};

struct NoNoise : NoiseGenerator {
    virtual float generate();
};

template<typename T> T clamp(T v, T minVal, T maxVal){
    return std::max(std::min(maxVal,v),minVal);
}
int randomInt(int lower, int upper);
float randomFloat(float lower, float upper);
bool randomCoin(float prop);



// ---- Noise generation
NoiseGenerator::NoiseGenerator() :
    generator(std::chrono::system_clock::now().time_since_epoch().count()) {
}

GaussianNoise::GaussianNoise(float mean, float stddev) : distribution(mean, stddev){

}
float GaussianNoise::generate(){
    float result = distribution(generator);
    return result;
}

CauchyNoise::CauchyNoise(float peak, float scale) : distribution(peak, scale){

}
float CauchyNoise::generate(){
    return distribution(generator);
}

float NoNoise::generate(){
    return 0;
}

int randomInt(int lower, int upper){ return rand() % (upper-lower+1) + lower; }
float randomFloat(float lower, float upper){ return lower + (upper-lower)*(float)rand()/(float)RAND_MAX; }
bool randomCoin(float prop){ return randomFloat(0,1) <= prop; }

