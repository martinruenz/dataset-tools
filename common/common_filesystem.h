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

#include <boost/filesystem.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

// ---- Filesystem
inline bool exists(const std::string& path) {
    return boost::filesystem::exists(path);
}

inline bool isDirectory(const std::string &path){
    //boost::filesystem::path bpath = path;
    //bpath.remove_trailing_separator()
    return boost::filesystem::is_directory(path);
}

inline bool createDirectory(const std::string &path){
    boost::filesystem::path dir(path);
    return boost::filesystem::create_directory(dir);
}

//// List all files if extensions is empty. Example: getFilenames(path, { ".jpg", ".png"});
inline std::vector<std::string> getFilenames(const std::string& path, std::vector<std::string> extensions, bool sorted=true) {
    using namespace boost::filesystem;
    using namespace boost::algorithm;
    std::vector<std::string> result;
    for(std::string& e : extensions) to_lower(e);
    for(auto it = directory_iterator(path); it != directory_iterator(); ++it){
        if (is_regular_file(it->status())){
            std::string ext = it->path().extension().string();
            to_lower(ext);
            if(extensions.size()==0 ||
                    std::find(begin(extensions), end(extensions), ext) != end(extensions))
                result.push_back(it->path().filename().string());
        }
    }
    if(sorted) std::sort(result.begin(), result.end());
    return result;
}

inline std::string getBasename(const std::string& filepath){
    return boost::filesystem::path(filepath).stem().string();
}


inline std::string getDirectory(const std::string &filepath){
    return boost::filesystem::path(filepath).parent_path().string() +
           boost::filesystem::path::preferred_separator;
}

inline std::string getRelativePath(const std::string &root, const std::string &path){
    // Could also be done using c++17: http://en.cppreference.com/w/cpp/filesystem/path/lexically_normal
    // return std::filesystem::path(root).lexically_relative(path).string();

    // This requires boost > 1.60
    boost::filesystem::path root_path(root);
    return boost::filesystem::path(path).lexically_relative(root_path.remove_trailing_separator()).string();

    // throw std::runtime_error("This function has no implementation due to missing libraries.");
    // return "";
}

//// Assumes that the index is trailing the filename
inline unsigned getFileIndex(const std::string& path) {
    std::string basename = getBasename(path);
    int i = basename.length()-1;
    while(i >= 0 && isdigit(basename[i])) i--;
    if(++i == (int)basename.length()) throw std::invalid_argument("Could not extract index of file: " + path);
    return std::stoi(basename.substr(i));
}


inline bool hasPrefix(const std::string &name, const std::string &prefix){
    if(name.size() < prefix.size()) return false;
    //if(name.substr(0,prefix.size()) == prefix) return true; slower than the next option:
    if(name.compare(0,prefix.size(),prefix) == 0) return true;
    return false;
}

inline std::vector<std::string> readFileLines(const std::string& path, bool removeEmpty=false){
    if(!exists(path)) throw std::invalid_argument("File does not exist: " + path);

    std::vector<std::string> result;

    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)){
        if(!removeEmpty || line != "")
            result.push_back(line);
    }

    return result;
}

inline std::vector<std::pair<std::string,std::string>> getFilePairs(const std::string& directory1, const std::string& directory2,
                                                             const std::string& prefix1, const std::string& prefix2,
                                                             const std::vector<std::string>& extensions1, const std::vector<std::string>& extensions2){
    if(!isDirectory(directory1))
        throw std::invalid_argument("Could not open directory: " + directory1);
    if(!isDirectory(directory2))
        throw std::invalid_argument("Could not open directory: " + directory2);

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

inline bool validateMatchingIndexes(const std::vector<std::pair<std::string, std::string> > &file_pairs){
    for(const auto& pair : file_pairs)
        if(getFileIndex(pair.first) != getFileIndex(pair.second)) return false;
    return true;
}
