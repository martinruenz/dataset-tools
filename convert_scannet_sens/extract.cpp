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
enum class RotationFormat { Matrix, Quaternion, Exponential };

int main(int argc, char* argv[])
{
    Parser parser(argc, argv);

    if(!parser.hasOption("-i")){
        cout << "A tool to extract scannet *.sens data.\n\n";
        cout << "Error, invalid arguments.\n"
                "Mandatory -i: input *.sens file\n"
                "Optional -ot: Output trajectory file.\n"
                "Optional -of: Output directory for rgb and depth frames.\n"
                "Optional --rotation_format: Either 'matrix', 'quaternion' or 'exponential' [default is exponential]\n"
                "\n"
                "Example: ./convert_scannet_sens -i <filename>.sens -ot <filename>.txt" << endl;

        return 1;
    }

    // Input data
    string filename = parser.getStringOption("-i");
    cout << "Loading data ... ";
    cout.flush();
    ml::SensorData sd(filename);
    cout << "done!" << endl;
    cout << sd << endl;

    // Prepare writing trajectories
    bool export_trajectory = parser.hasOption("-ot");
    bool export_frames = parser.hasOption("-of");

    string frames_directory = parser.getDirOption("-of");
    if(!exists(frames_directory)){
        createDirectory(frames_directory);
    }

    RotationFormat rotation_format = RotationFormat::Exponential;
    if(parser.hasOption("--rotation_format")){
        string format = parser.getStringOption("--rotation_format");
        boost::algorithm::to_lower(format);
        if(format == "matrix") rotation_format = RotationFormat::Matrix;
        else if(format == "quaternion") rotation_format = RotationFormat::Quaternion;
        else if(format == "exponential") rotation_format = RotationFormat::Exponential;
        else throw std::invalid_argument("unknown rotation format");
    }

    string trajectory_filename;
    std::ofstream trajectory_stream;
    const string sep = "\t";
    if(export_trajectory){
        trajectory_filename = parser.getStringOption("-ot");
        trajectory_stream.open(trajectory_filename);
    }

    Progress progress(sd.m_frames.size());

    #pragma omp parallel for num_threads(12) ordered schedule(static,1)
    for (size_t i = 0; i < sd.m_frames.size(); i++) {

        const ml::SensorData::RGBDFrame& frame = sd.m_frames[i];
        uint64_t ts = std::max(std::max(frame.getTimeStampColor(), frame.getTimeStampDepth()), i);

        //de-compress color and depth values
        if(export_frames){
            ml::vec3uc* colorData = sd.decompressColorAlloc(i);
            unsigned short* depthData = sd.decompressDepthAlloc(i);

            Mat depth_raw(sd.m_depthHeight, sd.m_depthWidth, CV_16UC1, depthData);
            Mat rgb(sd.m_colorHeight, sd.m_colorWidth, CV_8UC3, (unsigned char*)colorData);
            cvtColor(rgb, rgb, cv::COLOR_BGR2RGB);

            cv::imwrite(frames_directory + 'd' + std::to_string(ts) + ".png", depth_raw);
            cv::imwrite(frames_directory + "rgb" + std::to_string(ts) + ".jpg", rgb);

            free(colorData);
            free(depthData);
        }

        #pragma omp ordered
        {
            if(export_trajectory){
                ml::mat4f t = frame.getCameraToWorld();
                if(t.matrix[15] != 1 || t.matrix[14] != 0 || t.matrix[13] != 0 || t.matrix[12] != 0)
                    throw invalid_argument("sens file contains illegal transformation in frame ["+to_string(i)+"], last row of matrix: " +
                                           to_string(t.matrix[12]) + " " +
                                           to_string(t.matrix[13]) + " " +
                                           to_string(t.matrix[14]) + " " +
                                           to_string(t.matrix[15]));

                Eigen::Matrix3f rot;
                Eigen::Vector3f trans;
                rot << t.matrix[0], t.matrix[1], t.matrix[2],
                        t.matrix[4], t.matrix[5], t.matrix[6],
                        t.matrix[8], t.matrix[9], t.matrix[10];
                trans << t.matrix[3], t.matrix[7], t.matrix[11];

                trajectory_stream   << ts << sep << trans(0) << sep << trans(1) << sep << trans(2) << sep;

                //            if(true) {
                //                Eigen::Matrix3f flip = Eigen::AngleAxisf(M_PI, rot * Eigen::Vector3f::UnitX()).toRotationMatrix();
                //                rot = flip * rot;
                //            }
                switch (rotation_format) {
                case RotationFormat::Exponential: {
                    Eigen::AngleAxisf aa(rot);
                    Eigen::Vector3f v = aa.angle() * aa.axis();
                    trajectory_stream << v.x() << sep << v.y() << sep << v.z() << sep << "\n";
                    break;
                } case RotationFormat::Matrix: {
                    Eigen::IOFormat in_line_format(Eigen::StreamPrecision, Eigen::DontAlignCols, sep, sep);
                    trajectory_stream << rot.format(in_line_format) << "\n";
                    break;
                } case RotationFormat::Quaternion: {
                    Eigen::Quaternionf q(rot);
                    trajectory_stream << q.x() << sep << q.y() << sep << q.z() << sep << q.w() << "\n";
                    break;
                } default:
                    break;
                }
            }


            progress.show();
        }
    }

    if(export_trajectory){
        trajectory_stream.close();
    }

    cout << endl;

    return 0;
}
