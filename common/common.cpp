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

#include "common.h"

using namespace std;
using namespace cv;
//using namespace boost::filesystem;
//using namespace boost::algorithm;

bool Parser::initialised = false;
map<string,string> Parser::parameters = map<string,string>();

// ---- Image related
Mat floatToUC1(Mat image, float min, float max) {
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

void storeFloatImage(cv::Mat image, const std::string& path, float min, float max) {
    imwrite(path, floatToUC1(image, min, max));
}

// ---- String operations:
void splitString(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) elems.push_back(item);
    if (!ss && item.empty()) elems.push_back(""); // Check trailing (emtpy) item
}

vector<string> splitString(const string &s, char delim, bool allowEmpty) {
    vector<string> strings;
    splitString(s, delim, strings);
    if(!allowEmpty) strings.erase(std::remove(strings.begin(), strings.end(), ""), strings.end());
    return strings;
}

// See: http://stackoverflow.com/questions/5891610/how-to-remove-characters-from-a-string
void removeChars(string& str, char* charsToRemove) {
    for(unsigned int i = 0; i < strlen(charsToRemove); ++i)
        str.erase(remove(str.begin(), str.end(), charsToRemove[i]), str.end());
}

Vec3b stringToColor(const std::string& color){
    vector<string> rgbs = splitString(color, ',', false);
    vector<int> rgb;
    for(auto& c : rgbs) rgb.push_back(stoi(c));
    if(rgb.size() != 3) throw std::invalid_argument("Colour value not recognised.");
    for(int c : rgb) if(c < 0 || c > 255) throw std::invalid_argument("Colour value out of bounds.");
    return Vec3b(rgb[2],rgb[1],rgb[0]); //OpenCV is BGR
}

Eigen::VectorXf stringToEigenVec(const std::string& color){
    vector<string> floats = splitString(color, ',', false);
    Eigen::VectorXf result(floats.size());
    for (size_t i = 0; i < floats.size(); ++i) result(i) = stof(floats[i]);
    return result;
}

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
float randomCoin(float prop){ return randomFloat(0,1) < prop; }

string cvTypeToString(int type) {
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


// ---- Various
//int clamp(int v, int minVal, int maxVal) {
//    return max(min(maxVal,v),minVal);
//}

// ---- Terminal output
void showProgress(float p) {
    // See: http://stackoverflow.com/questions/14539867/how-to-display-a-progress-indicator-in-pure-c-c-cout-printf
    const int barWidth = 50;
    std::cout << "[";
    int pos = barWidth * p;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(p * 100.0) << " %\r";
    std::cout.flush();
}

// ---- Filesystem
bool exists(const std::string& path) {
    return boost::filesystem::exists(path);
}

bool isDirectory(const string &path){
    //boost::filesystem::path bpath = path;
    //bpath.remove_trailing_separator()
    return boost::filesystem::is_directory(path);
}

std::vector<string> getFilenames(const std::string& path, vector<string> extensions, bool sorted) {
    using namespace boost::filesystem;
    using namespace boost::algorithm;
    std::vector<string> result;
    for(string& e : extensions) to_lower(e);
    for(auto it = directory_iterator(path); it != directory_iterator(); ++it){
        if (is_regular_file(it->status())){
            string ext = it->path().extension().string();
            to_lower(ext);
            if(extensions.size()==0 ||
                    find(begin(extensions), end(extensions), ext) != end(extensions))
                result.push_back(it->path().filename().string());
        }
    }
    if(sorted) std::sort(result.begin(), result.end());
    return result;
}

string getBasename(const string& filepath){
    return boost::filesystem::path(filepath).stem().string();
}

unsigned getFileIndex(const string& path) {
    string basename = getBasename(path);
    int i = basename.length()-1;
    while(i >= 0 && isdigit(basename[i])) i--;
    if(++i == (int)basename.length()) throw invalid_argument("Could not extract index of file: " + path);
    return stoi(basename.substr(i));
}


bool hasPrefix(const string &name, const string &prefix){
    if(name.size() < prefix.size()) return false;
    //if(name.substr(0,prefix.size()) == prefix) return true; slower than the next option:
    if(name.compare(0,prefix.size(),prefix) == 0) return true;
    return false;
}

std::vector<std::pair<std::string,std::string>> getFilePairs(const std::string& directory1, const std::string& directory2,
                                                             const std::string& prefix1, const std::string& prefix2,
                                                             const std::vector<std::string>& extensions1, const std::vector<std::string>& extensions2){
    if(!isDirectory(directory1))
        throw invalid_argument("Could not open directory: " + directory1);
    if(!isDirectory(directory2))
        throw invalid_argument("Could not open directory: " + directory2);

    std::vector<std::pair<std::string,std::string>> result;
    std::vector<std::string> files_unfiltered1 = getFilenames(directory1, extensions1);
    std::vector<std::string> files_unfiltered2 = getFilenames(directory2, extensions2);
    size_t i1=0,i2=0;
    for(; i1 < files_unfiltered1.size() && i2 < files_unfiltered2.size();) {
        if(!hasPrefix(files_unfiltered1[i1], prefix1)){
            i1++;
            continue;
        }
        if(!hasPrefix(files_unfiltered2[i2], prefix2)){
            i2++;
            continue;
        }
        result.push_back(std::make_pair(files_unfiltered1[i1++],files_unfiltered2[i2++]));
    }
    return result;
}

bool validateMatchingIndexes(const std::vector<std::pair<string, string> > &file_pairs){
    for(const auto& pair : file_pairs)
        if(getFileIndex(pair.first) != getFileIndex(pair.second)) return false;
    return true;
}
