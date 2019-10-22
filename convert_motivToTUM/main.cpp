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

#include "../common/common.h"
#include <fstream>

using namespace std;

struct Pose {
    double ts; // double in seconds
    Eigen::Vector3d t;
    Eigen::Quaterniond r;
};

struct RigidObject {
    vector<Pose> poses;
    string name;
    int num_markers = 0;
    int id;

    long num_frames_tracked = 0;
    long num_frames_untracked = 0;
    long consecutive_untracked = 0;
    long highest_consecutive_untracked = 0;
};

int main(int argc, char * argv[])
{
    Parser parser(argc, argv);

    if(!parser.hasOption("--in") ||
            !parser.hasOption("--outdir")){
        cout << "Error, invalid arguments.\n"
                "Mandatory --in: Path to directory containing containing depth images.\n"
                "Mandatory --outdir: Output directory (has to exist).\n";
        return 1;
    }

    string in_path = parser.getOption("--in");
    string out_path = parser.getDirOption("--outdir");

    if(!exists(in_path)){
        cerr << "Input file not found." << endl;
        return 1;
    }

    if(!isDirectory(out_path) || !exists(out_path)) {
        cerr << "Output directory not available." << endl;
        return 1;
    }

    long num_frames = -1;
    long num_bodies = -1;
    string version;
    map<int, RigidObject> objects; // ID, Object-data

    string line;
    ifstream file(in_path);

    // Reading header
    while (std::getline(file, line)){

        vector<string> cells = splitString(line, ',', false);

        if(cells.empty()){
            cerr << "ERROR: Unknown file format." << endl;
            return 1;
        }

        // Ignore comments
        if(cells[0] == "comment"){
            continue;
        }

        // read / print info
        if(cells[0] == "info"){
            if(cells.size() == 3){
                cout << cells[1] << ": " << cells[2] << "\n";
                if(cells[1] == "version") {
                    version = cells[2];
                } else if(cells[1] == "framecount") {
                    num_frames = stol(cells[2]);
                } else if(cells[1] == "rigidbodycount") {
                    num_bodies = stol(cells[2]);
                    break;
                }
            }
            continue;
        }
    }

    // Reading object-header
    for(int i=0; i<num_bodies; i++){
        if(!std::getline(file, line)){
            cerr << "ERROR in file format (expected more data)." << endl;
            return 1;
        }

        vector<string> cells = splitString(line, ',', false);
        //objects.emplace_back();
        //RigidObject& object = objects.back();

        RigidObject object;

        if(cells.size() < 4){
            cerr << "ERROR: Unknown file format." << endl;
            return 1;
        }

        if(cells[0] != "rigidbody"){
            cerr << "ERROR: Unknown file format." << endl;
            return 1;
        }

        object.name = cells[1];
        object.id = stoi(cells[2]);
        object.num_markers = stoi(cells[3]);
        removeChars(object.name, "\"");
        objects[object.id] = object;

        if(object.name.size() == 0){
            cerr << "ERROR: Unnamed objects are not supported." << endl;
            return 1;
        }

        cout << "Object '" << object.name << "' (id: " << object.id << ") has " << object.num_markers << " markers.\n";
    }

    // Reading frames
    long data_rows = (num_bodies+1)*num_frames;
    for(int i=0; i<data_rows; i++){
        if(!std::getline(file, line)){
            cerr << "ERROR in file format (expected more data)." << endl;
            return 1;
        }

        vector<string> cells = splitString(line, ',', false);

        if(cells.size() < 4){
            cerr << "ERROR in file format." << endl;
            return 1;
        }

        if(cells[0] == "frame") {

            double ts = stod(cells[2]);
            long index = stol(cells[1]);
            unsigned tracked_objects = stoi(cells[3]);

            if(cells.size() < 4 + tracked_objects * 11){
                cerr << "ERROR in file format." << endl;
                return 1;
            }

            for (unsigned i = 0; i < tracked_objects; ++i) {
                unsigned col = 4+11*i;
                int id = stoi(cells[col]);
                RigidObject& object = objects[id];
                object.poses.push_back({
                                           ts,
                                           Eigen::Vector3d(stod(cells[col+1]),stod(cells[col+2]),stod(cells[col+3])),
                                           Eigen::Quaterniond(stod(cells[col+7]),stod(cells[col+5]),stod(cells[col+6]),stod(cells[col+4]))
                                       });
            }

        }

        if(cells[0] == "rigidbody") {

            // rigid-body has at least 7 columns
            if(cells.size() < 7){
                cerr << "ERROR in file format." << endl;
                return 1;
            }

            string name = cells[3];
            int id = stoi(cells[4]);
            RigidObject& object = objects[id];
            removeChars(name, "\"");
            if(object.name != name) {
                cerr << "ERROR in file format." << endl;
                return 1;
            }

            long last_tracked = stol(cells[5]);
            int num_markers = stoi(cells[6]);

            if(last_tracked > 0) {
                object.num_frames_untracked++;
                object.consecutive_untracked++;
                if(object.consecutive_untracked > object.highest_consecutive_untracked)
                    object.highest_consecutive_untracked = object.consecutive_untracked;
            } else {
                object.num_frames_tracked++;
                object.consecutive_untracked = 0;
            }
        }
    }

    // Write output files
    for(const auto& i : objects){
        const RigidObject& o = i.second;

        string outfile = out_path + o.name + ".log";
        if(exists(outfile)){
          cerr << "Out file already exists.\n" << endl;
          return 1;
        }

        std::ofstream ostream(outfile);
        const string sep = "\t";
        for(const Pose& p : o.poses){
            ostream << p.ts << sep
                    << p.t(0) << sep << p.t(1) << sep << p.t(2) << sep
                    << p.r.x() << sep << p.r.y() << sep << p.r.z() << sep << p.r.w() << "\n";
        }
        ostream.close();
    }

    // Print statistics
    cout << "Statistics:\n";
    for(const auto& i : objects){
        const RigidObject& o = i.second;
        if(o.num_frames_tracked + o.num_frames_untracked != num_frames){
            cerr << "ERROR in file: Object '" << o.name << "' not listed in all frames." << endl;
            return 1;
        }
        cout << o.name << " was tracked in " << o.num_frames_tracked << "/" << num_frames
             << " frames (" << 100 * o.num_frames_tracked / float(num_frames) << "%)."
             << " Biggest gap: " << o.highest_consecutive_untracked << endl;
    }

    // Sanity check, left-over data?
    if(std::getline(file, line)){
        cerr << "ERROR in file format (unexpected rows)." << endl;
        return 1;
    }

    return 0;
}
