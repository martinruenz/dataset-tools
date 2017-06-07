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

#ifdef NDEBUG
    #define RELEASE
#endif

#ifdef RELEASE
    #define EIGEN_NO_DEBUG
    #define CHECK(x) x
    #define CHECK_THROW(x) if(!x) throw std::invalid_argument("Error: Unmet condition.");
#else
    #define DEBUG
    #define CHECK(x) assert(x)
    #define CHECK_THROW(x) assert(x)
#endif

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

#include <boost/filesystem.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

#include <eigen3/Eigen/Dense>

//using namespace boost::filesystem;
//using namespace boost::algorithm;

// ---- Input parameters
class Parser {
public:

    static void init(int argc, char* argv[], bool verbose=false) {
        initialised = true;
        std::string key;
        std::string val;
        for (int i = 0; i < argc; ++i) {
            if(argv[i][0] == '-') {
                if(key!="") parameters[key] = val;
                key = argv[i];
                val = "";
            } else {
                val += argv[i];
            }
        }
        if(key!="") parameters[key] = val;
        if(verbose)
            for(auto p : parameters)
                std::cout << "k: " << p.first << "  v: " << p.second << std::endl;
    }

    static bool hasOption(const std::string& option){
        assert(initialised);
        return parameters.find(option) != parameters.end();
    }

    static std::string getOption(const std::string& option){
        assert(initialised);
        std::map<std::string,std::string>::iterator it = parameters.find(option);
        if(it != parameters.end()) return it->second;
        return "";
    }

    static std::string getPathOption(const std::string& option){
        return getOption(option) + '/';
    }

    static float getFloatOption(const std::string& option, float default_value = 0){
        return hasOption(option) ? stof(getOption(option)) : default_value;
    }

    static float getDoubleOption(const std::string& option, double default_value = 0){
        return hasOption(option) ? stod(getOption(option)) : default_value;
    }

    static int getIntOption(const std::string& option, int default_value = 0){
        return hasOption(option) ? stoi(getOption(option)) : default_value;
    }

    static std::string getStringOption(const std::string& option, const std::string& default_value = ""){
        return hasOption(option) ? getOption(option) : default_value;
    }

    static uchar getUCharOption(const std::string& option, uchar default_value = 0){
        int val = hasOption(option) ? stoi(getOption(option)) : default_value;
        if(val > 255) throw std::invalid_argument("UChar option is out of bounds.");
        return (uchar)val;
    }

private:
    static bool initialised;
    static std::map<std::string,std::string> parameters;
};

// -- Types
// Like boost optional or std::experimental::optional
template<typename T>
struct FailableResult{
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    FailableResult(T d) : data(d), valid(true) {}
    FailableResult() : valid(false) {}
    T data;
    bool valid;
};

// ---- Image related
cv::Mat floatToUC1(cv::Mat image, float min = 0, float max = 255);
void storeFloatImage(cv::Mat image, const std::string& path, float min = 0, float max = 255);
// ---- String operations:
void splitString(const std::string &s, char delim, std::vector<std::string> &elems);

std::vector<std::string> splitString(const std::string &s, char delim, bool allowEmpty = true);

// See: http://stackoverflow.com/questions/5891610/how-to-remove-characters-from-a-string
void removeChars(std::string& str, char* charsToRemove);

cv::Vec3b stringToColor(const std::string& color);

Eigen::VectorXf stringToEigenVec(const std::string& color);

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
float randomCoin(float prop);

std::string cvTypeToString(int type);


// ---- Various
//int clamp(int v, int minVal, int maxVal) {
//    return max(min(maxVal,v),minVal);
//}

// ---- Terminal output
void showProgress(float p);

// ---- Filesystem
bool exists(const std::string& path);
bool isDirectory(const std::string& path);

// List all files if extensions is empty. Example: getFilenames(path, { ".jpg", ".png"});
std::vector<std::string> getFilenames(const std::string& path, std::vector<std::string> extensions, bool sorted=true);

// Return the basename of a file
std::string getBasename(const std::string& filepath);

// Assumes that the index is trailing the filename
unsigned getFileIndex(const std::string& path);

bool hasPrefix(const std::string& name, const std::string& prefix);

std::vector<std::pair<std::string,std::string>> getFilePairs(const std::string& directory1, const std::string& directory2,
                                                             const std::string& prefix1, const std::string& prefix2,
                                                             const std::vector<std::string>& extensions1, const std::vector<std::string>& extensions2);

bool validateMatchingIndexes(const std::vector<std::pair<std::string,std::string>>& file_pairs);
