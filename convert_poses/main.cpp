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
#include "../common/common_3d.h"

using namespace std;
using namespace cv;

#if 0 // Convert a coordinate to each coordinate frame:
int main(int argc, char * argv[])
{
    Parser::init(argc, argv, true);

    if(!Parser::hasOption("--file") || !Parser::hasOption("--out") || !Parser::hasOption("--center")){
        cerr << "Error, invalid arguments.\n";
        return 0;
    }

    string infile = Parser::getOption("--file");
    string outfile = Parser::getOption("--out");

    if(exists(outfile)){
        cerr << "Out file already exists.\n";
        return 0;
    }

    if(!exists(infile)){
        cerr << "Could not find input file.\n";
        return 0;
    }

    Eigen::Vector3f center = stringToEigenVec(Parser::getOption("--center"));
    ofstream out_stream(outfile);

    string line;
    ifstream in_stream(infile);
    double ts, tx, ty, tz, qx, qy, qz, qw;
    while (getline(in_stream, line)){
        if(!isdigit(line[0]) && line[0] != '-') continue;
        istringstream iss(line);
        iss >> ts >> tx >> ty >> tz >> qx >> qy >> qz >> qw;

        Eigen::Vector3f t(tx,ty,tz);
        Eigen::Quaternionf q(qw,qx,qy,qz);

        Eigen::Vector3f transformed = q*center + t;
        out_stream << ts << " " << transformed(0)  << " " << transformed(1)  << " " << transformed(2) << "\n";
    }

    out_stream.close();
    in_stream.close();

    return 1;
}
#endif

Pose extractPoseFromFile(const std::string& file, int timestamp){
    string line;
    ifstream in_stream(file);
    int tick;
    double tx, ty, tz, qx, qy, qz, qw;
    while (getline(in_stream, line)){
        if(!isdigit(line[0]) && line[0] != '-') continue;
        istringstream iss(line);
        iss >> tick >> tx >> ty >> tz >> qx >> qy >> qz >> qw;

        if(tick == timestamp){
            in_stream.close();
            return Pose(Eigen::Vector3d(tx,ty,tz),Eigen::Quaterniond(qw,qx,qy,qz));
        }
    }

    in_stream.close();
    throw std::invalid_argument("Could not find timestamp " + std::to_string(timestamp) + " -- wrong frame number?");
}

int main(int argc, char * argv[])
{
    Parser::init(argc, argv, true);


    if(!Parser::hasOption("--frame")  || // frame number
       !Parser::hasOption("--object") || // object poses (file with ts, tx, ty, tz, qx, qy, qz, qw)
       !Parser::hasOption("--camera") || // camera / global poses
       !Parser::hasOption("--gtobject") || // ground-truth object poses
       !Parser::hasOption("--gtcamera") || // ground-truth camera poses
       !Parser::hasOption("--out")){ // output file
        cerr << "Error, invalid arguments.\n";
        return 1;
    }

    string objectfile = Parser::getOption("--object");
    string camerafile = Parser::getOption("--camera");
    string gtobjectfile = Parser::getOption("--gtobject");
    string gtcamerafile = Parser::getOption("--gtcamera");
    string outfile = Parser::getOption("--out");
    int frame = Parser::getIntOption("--frame");

    if(exists(outfile)){
        cerr << "Out file already exists.\n";
        return 2;
    }

    if(!exists(objectfile) || !exists(camerafile) ||
       !exists(gtobjectfile) || !exists(gtcamerafile)){
        cerr << "Could not find one of the input files.\n";
        return 1;
    }

    Pose gtPoseCamAlign = extractPoseFromFile(gtcamerafile, frame);
    Pose gtPoseObjAlign = extractPoseFromFile(gtobjectfile, frame);
    Pose exPoseCamAlign = extractPoseFromFile(camerafile, frame);
    Pose exPoseObjAlign = extractPoseFromFile(objectfile, frame);

    //Eigen::Matrix4d centerInExport = exPoseObjAlign.toMatrix().inverse() * exPoseCamAlign.toMatrix() *
    //                                 gtPoseCamAlign.toMatrix().inverse() * gtPoseObjAlign.toMatrix();

    //Eigen::Matrix4d centerInExport = exPoseObjAlign.toMatrix().inverse() * exPoseCamAlign.toMatrix() *
    //                                 gtPoseCamAlign.toMatrix() * gtPoseObjAlign.toMatrix();

    Eigen::Matrix4d centerInExport = gtPoseObjAlign.toMatrix();
    cout << "\ncenter in gt-wor is at: \n" << centerInExport;
    centerInExport = gtPoseCamAlign.toMatrix().inverse() * centerInExport;
    cout << "\ncenter in gt-cam is at: \n" << centerInExport;
    centerInExport = exPoseCamAlign.toMatrix() * centerInExport;
    cout << "\ncenter in ex-world is at: \n" << centerInExport;
    centerInExport = exPoseObjAlign.toMatrix().inverse() * centerInExport;
    cout << "\ncenter in ex-object is at: \n" << centerInExport;

    string line;
    ofstream out_stream(outfile);
    ifstream in_stream(objectfile);
    int tick;
    double tx, ty, tz, qx, qy, qz, qw;
    while (getline(in_stream, line)){
        if(!isdigit(line[0]) && line[0] != '-') continue;
        istringstream iss(line);
        iss >> tick >> tx >> ty >> tz >> qx >> qy >> qz >> qw;

        Pose converted(Pose(Eigen::Vector3d(tx,ty,tz),Eigen::Quaterniond(qw,qx,qy,qz)).toMatrix() * centerInExport);

        out_stream << tick << " " << converted.t(0) << " " << converted.t(1)  << " " << converted.t(2) << " " <<
                      converted.q.x() << " " << converted.q.y() << " " << converted.q.z() << " " << converted.q.w() << "\n";
    }

    out_stream.close();
    in_stream.close();

    return 0;
}
