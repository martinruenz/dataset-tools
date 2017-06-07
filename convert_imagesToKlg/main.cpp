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

/**
 * See: https://github.com/mp3guy/Logger2/blob/master/src/Logger2.h
 *
 * Format is:
 * int32_t at file beginning for frame count
 *
 * For each frame:
 * int64_t: timestamp
 * int32_t: depthSize
 * int32_t: imageSize
 * depthSize * unsigned char: depth_compress_buf
 * imageSize * unsigned char: encodedImage->data.ptr
 */

#include "../common/common.h"
#include <fstream>

using namespace std;
using namespace cv;

int main(int argc, char * argv[])
{
    Parser::init(argc, argv);

    if(!Parser::hasOption("--out") ||
       !Parser::hasOption("--depthdir") ||
       !Parser::hasOption("--rgbdir")){
      cout << "Error, invalid arguments.\n"
              "Mandatory --depthdir: Path to directory containing containing depth images.\n"
              "Mandatory --rgbdir: Path to directory containing rgb images.\n"
              "Mandatory --out: Output klg path.\n"
              "Optional --fps: Frames per second (default: 24.00).\n"
              "Optional -s: Factor, which scales depth values to [m] (default: 1.00).\n";
      return 1;
    }

    string dirRGB = Parser::getPathOption("--rgbdir");
    string dirDepth = Parser::getPathOption("--depthdir");

    vector<string> inputRGBs = getFilenames(dirRGB, { ".jpg", ".png"});
    vector<string> inputDepths = getFilenames(dirDepth, { ".exr", ".png"});
    string outfile = Parser::getOption("--out");
    float depthScale = Parser::getFloatOption("-s", 1.0);
    float fps = Parser::getFloatOption("-fps", 24.0);

    if(inputRGBs.size() == 0 || inputRGBs.size() != inputDepths.size()) {
      cerr << "Input empt or not matching." << endl;
      return 1;
    }

    if(exists(outfile)){
      cerr << "Out file already exists.\n" << endl;
      return 2;
    }

    float progressStep = 1.0 / (inputRGBs.size()+1);
    int32_t frameCount = inputRGBs.size();
    int64_t timeStep = 1000000 / fps;
    int64_t timestamp = 0;
    std::ofstream outStream(outfile, std::ofstream::binary);

    outStream.write((char*)&frameCount, sizeof(frameCount));

    for(size_t i = 0; i < inputRGBs.size(); i++) {

        showProgress(i*progressStep);

        const string& pathRGB = dirRGB + inputRGBs[i];
        const string& pathDepth = dirDepth + inputDepths[i];

        if(getFileIndex(pathRGB) != getFileIndex(pathDepth))
          throw std::invalid_argument("RGB and Depth indexes are not matching.");

        // Load input
        cv::Mat rgb = imread(pathRGB);
        cv::cvtColor(rgb, rgb, CV_RGB2BGR);
        cv::Mat depth = imread(pathDepth, cv::IMREAD_UNCHANGED);

        if(rgb.total() == 0) throw std::invalid_argument("Could not read rgb-image file: " + pathRGB);
        if(depth.total() == 0) throw std::invalid_argument("Could not read depth-image file: " + pathDepth);
        if(rgb.total() != depth.total()) throw std::invalid_argument("Image sizes are not matching.");
        if(!rgb.isContinuous() || !depth.isContinuous()) throw std::invalid_argument("Data has to be continuous.");

        if(!(depthScale == 0.001 && depth.type() == CV_16UC1)) depth.convertTo(depth, CV_16UC1, 1000 * depthScale);

        // Write frame header
        int32_t depthSize = depth.total() * depth.elemSize();
        int32_t rgbSize = rgb.total() * rgb.elemSize();
        outStream.write((char*)&timestamp, sizeof(timestamp));
        outStream.write((char*)&depthSize, sizeof(depthSize));
        outStream.write((char*)&rgbSize, sizeof(rgbSize));

        // Write frame data
        outStream.write((char*)depth.data, depthSize);
        outStream.write((char*)rgb.data, rgbSize);

        timestamp += timeStep;
    }

    outStream.close();

    return 0;
}
