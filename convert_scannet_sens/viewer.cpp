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

#include <cstddef>
#include <iostream>
#include "../common/common.h"
#include "../external/scannet/sensorData.h"

using namespace std;
using namespace cv;

typedef std::chrono::time_point<std::chrono::system_clock> Time;
typedef std::chrono::duration<double> Duration;
typedef std::chrono::system_clock Clock;

int main(int argc, char* argv[])
{
    Parser parser(argc, argv);

    if(!parser.hasOption("-i")){
        cout << "A tool to extract scannet *.sens data.\n\n";
        cout << "Error, invalid arguments.\n"
                "Mandatory -i: input *.sens file\n"
                "\n"
                "Example: ./convert_scannet_sens -i <filename>.sens" << endl;

        return 1;
    }

    // Input data
    string filename = parser.getStringOption("-i");
    cout << "Loading data ... ";
    cout.flush();
    ml::SensorData sd(filename);
    cout << "done!" << endl;
    cout << sd << endl;

    for (size_t i = 0; i < sd.m_frames.size(); i++) {
        Time t = Clock::now();

        //de-compress color and depth values
        ml::vec3uc* colorData = sd.decompressColorAlloc(i);
        unsigned short* depthData = sd.decompressDepthAlloc(i);

        Mat depth(sd.m_depthHeight, sd.m_depthWidth, CV_16UC1, depthData);
        Mat rgb(sd.m_colorHeight, sd.m_colorWidth, CV_8UC3, (unsigned char*)colorData);
        cvtColor(rgb, rgb, cv::COLOR_BGR2RGB);

        depth.convertTo(depth, CV_16UC1, 20);
        cv::imshow("Depth", depth);
        cv::waitKey(1);
        cv::imshow("RGB", rgb);
        cv::waitKey(1);

        free(colorData);
        free(depthData);

        Duration d = Clock::now() - t;
        cout << "Playback speed: " <<  1.0f / d.count() << "Hz\r";
        cout.flush();
    }

    cout << endl;
    return 0;
}
