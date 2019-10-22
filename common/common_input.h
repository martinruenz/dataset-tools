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
#include "common_strings.h"

#include <iostream>
#include <vector>
#include <map>

#include <boost/filesystem.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// ---- Terminal output
inline void showProgress(float p) {
    // See: http://stackoverflow.com/questions/14539867/how-to-display-a-progress-indicator-in-pure-c-c-cout-printf
    const int barWidth = 50;
    std::cout << "[";
    int pos = int(barWidth * p);
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(p * 100.0f) << " %\r";
    std::cout.flush();
}
struct Progress {
    float step;
    unsigned i;
    Progress(size_t iterations){
        if(iterations==0) step = 1;
        else step = 1.f / iterations;
        i = 0;
    }
    inline void show(){
        showProgress(++i*step);
    }
};


// ---- Input parser
class Parser {
public:

    inline Parser(int argc, char* argv[], bool verbose=false){
        init(argc, argv, verbose);
    }

    inline void init(int argc, char* argv[], bool verbose=false) {
        initialised = true;
        std::string key;
        std::string val;
        for (int i = 0; i < argc; ++i) {
            if(argv[i][0] == '-') {
                if(key!="") parameters[key] = val;
                key = argv[i];
                val = "";
            } else {
                if(val != "") val += ' ';
                val += argv[i];
            }
        }
        if(key!="") parameters[key] = val;
        if(verbose)
            for(auto p : parameters)
                std::cout << "k: " << p.first << "  v: " << p.second << std::endl;
    }

    inline bool hasOption(const std::string& option){
        assert(initialised);
        return parameters.find(option) != parameters.end();
    }

    inline std::string getOption(const std::string& option){
        assert(initialised);
        std::map<std::string,std::string>::iterator it = parameters.find(option);
        if(it != parameters.end()) return it->second;
        return "";
    }

    inline std::string getDirOption(const std::string& option, bool check_exists=false){
        std::string result = getOption(option);
        if(result!="") result += '/';
        if(check_exists && !boost::filesystem::is_directory(result))
            throw std::invalid_argument("Directory does not exist: " + result);
        return result;
    }

    inline boost::filesystem::path getBoostPathOption(const std::string& option){
        boost::filesystem::path path(getOption(option));
        return path.remove_trailing_separator();
    }

    inline float getFloatOption(const std::string& option, float default_value = 0){
        return hasOption(option) ? stof(getOption(option)) : default_value;
    }

    inline float getDoubleOption(const std::string& option, double default_value = 0){
        return hasOption(option) ? stod(getOption(option)) : default_value;
    }

    inline int getIntOption(const std::string& option, int default_value = 0){
        return hasOption(option) ? stoi(getOption(option)) : default_value;
    }

    inline std::string getStringOption(const std::string& option, const std::string& default_value = ""){
        return hasOption(option) ? getOption(option) : default_value;
    }

    inline uchar getUCharOption(const std::string& option, uchar default_value = 0){
        int val = hasOption(option) ? stoi(getOption(option)) : default_value;
        if(val > 255) throw std::invalid_argument("UChar option is out of bounds.");
        return (uchar)val;
    }

    inline cv::Vec3b getRgbOption(const std::string& option){
        return stringToColor(getOption(option), false);
    }

    static bool askYesNo(const std::string& question){
        std::string input;
        std::cout << question << " [y/n]" << std::endl;
        std::cin >> input;
        std::transform(input.begin(), input.end(), input.begin(), ::tolower);
        if(input == "y" || input == "yes") return true;
        return false;
    }

private:
    bool initialised = false;
    std::map<std::string,std::string> parameters;
};
