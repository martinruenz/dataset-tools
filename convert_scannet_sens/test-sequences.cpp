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

#include <iostream>
#include <sstream>
#include "../external/scannet/sensorData.h"

using namespace std;

string mat4f_to_string(const ml::mat4f& m){
    stringstream str;
    str << "["
            << m.matrix[0] << ", " << m.matrix[1] << ", " << m.matrix[2] << ", " <<m.matrix[3] << "]["
            << m.matrix[4] << ", " << m.matrix[5] << ", " << m.matrix[6] << ", " <<m.matrix[7] << "]["
            << m.matrix[8] << ", " << m.matrix[9] << ", " << m.matrix[10] << ", " <<m.matrix[11] << "]["
            << m.matrix[12] << ", " << m.matrix[13] << ", " << m.matrix[14] << ", " <<m.matrix[15] << "]]";
    return str.str();
}

tuple<bool,bool,ml::mat4f,ml::mat4f> analyze_sens(const string& input, bool verbose = false){

    // Input
    if(verbose){
        cout << "Loading data ... ";
        cout.flush();
    }
    ml::SensorData sd(input);
    if(verbose){
        cout << "done!" << endl;
        cout << sd << endl;
    }

    // Stats
    bool ts_d_monotonic = true;
    bool ts_c_monotonic = true;
    bool ts_d_available = false;
    bool ts_c_available = false;
    uint64_t ts_d_last = 0;
    uint64_t ts_c_last = 0;

    bool has_illegal_transformation = false;

    for (size_t i = 0; i < sd.m_frames.size(); i++) {

        // Test timestamps
        const ml::SensorData::RGBDFrame& frame = sd.m_frames[i];
        uint64_t t_d = frame.getTimeStampDepth();
        uint64_t t_c = frame.getTimeStampColor();
        if (t_d > 0) ts_d_available = true;
        if (t_c > 0) ts_c_available = true;
        if (t_d < ts_d_last) ts_d_monotonic = false;
        if (t_c < ts_c_last) ts_c_monotonic = false;
        ts_d_last = t_d;
        ts_c_last = t_c;

        // Test poses
        ml::mat4f t = frame.getCameraToWorld();
        if(t.matrix[15] != 1 || t.matrix[14] != 0 || t.matrix[13] != 0 || t.matrix[12] != 0){
            has_illegal_transformation = true;
            if(verbose) cout << "Found illegal transformation at frame " << to_string(i) << ": " << mat4f_to_string(t) << endl;
        }
    }

    if(verbose){
        cout << "Depth timestamps are monotonic: " << (ts_d_monotonic ? "\x1B[32m yes" : "\x1B[31m no") << "\x1B[0m \n";
        cout << "RGB   timestamps are monotonic: " << (ts_c_monotonic ? "\x1B[32m yes" : "\x1B[31m no") << "\x1B[0m \n";
        cout << "Depth timestamps are available: " << (ts_d_available ? "\x1B[32m yes" : "\x1B[31m no") << "\x1B[0m \n";
        cout << "RGB   timestamps are available: " << (ts_c_available ? "\x1B[32m yes" : "\x1B[31m no") << "\x1B[0m \n";
        cout << "All  camera  poses  were legal: " << (!has_illegal_transformation ? "\x1B[32m yes" : "\x1B[31m no") << "\x1B[0m \n";
        cout << endl;
    }
    return make_tuple(!ts_d_monotonic || !ts_c_monotonic, has_illegal_transformation, sd.m_calibrationDepth.m_intrinsic, sd.m_calibrationColor.m_intrinsic);
}

int main(int argc, char* argv[])
{
    if(argc < 2 || argc > 3) {
        cout << "A tool to analyse scannet *.sens data.\n\n"
                "Error, invalid arguments.\n"
                "Mandatory: input *.sens file / input *.txt file\n"
                "Optional path to dataset dir (if txt is provided)."
             << endl;
        return 1;
    }

    // Input data
    string filename = argv[1];
    if(filename.substr(filename.find_last_of(".") + 1) == "txt"){
        // Analyse many sens files
        string sequence_name;
        vector<ml::mat4f> calibration_depth;
        vector<ml::mat4f> calibration_color;
        string root = (argc == 3 ? argv[2] : "");
        ifstream in_stream(filename);
        while (getline(in_stream, sequence_name)){
            cout << "Checking " << sequence_name << "...";
            cout.flush();
            tuple<bool,bool,ml::mat4f,ml::mat4f> r = analyze_sens(root + "/" + sequence_name + "/" + sequence_name  + ".sens");

            if(get<0>(r))
                cout << "\x1B[31m Timestamp issue \x1B[0m";
            else
                cout << "\x1B[32m Timestamps good \x1B[0m";

            if(get<1>(r))
                cout << "\x1B[31m Pose issue \x1B[0m";
            else
                cout << "\x1B[32m Poses good \x1B[0m";

            cout << endl;
            cout << mat4f_to_string(get<2>(r)) << endl;
            cout << mat4f_to_string(get<3>(r)) << endl;
            calibration_depth.push_back(get<2>(r));
            calibration_color.push_back(get<3>(r));
        }
        in_stream.close();
        ofstream out_cal_d("cal-depth.txt");
        ofstream out_cal_c("cal-color.txt");
        for(auto& m : calibration_depth) out_cal_d << mat4f_to_string(m) << "\n";
        for(auto& m : calibration_color) out_cal_c << mat4f_to_string(m) << "\n";
        out_cal_d.close();
        out_cal_c.close();
    } else {
        // Analyse single sens files
        analyze_sens(filename, true);
    }

    return 0;
}
