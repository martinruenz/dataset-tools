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

// Input: Projective depth values (distances to camera center)
// Output: Depth values (z-component)
Mat convertDepth(Mat input, const PinholeParameters& intrinsics){

    if(input.type() != CV_32FC3) // convert fx or fy to f and test
    {
        cerr << "Error, wrong image format in makeDepthProjective()." << endl;
        return Mat();
    }

    PinholeCamera cam(intrinsics);

    Mat result(input.rows, input.cols, CV_32FC1);

    for (int i = 0; i < input.rows; ++i){
        cv::Vec3f* pixel = input.ptr<cv::Vec3f>(i);
        float* pOut = result.ptr<float>(i);
        for (int j = 0; j < input.cols; ++j){
            Vec3d ray = cam.toRay(Vec2d(j,i));
            ray *= pixel[j][0];
            pOut[j] = ray[2];
        }
    }
    return result;
}

// See: 'A Benchmark for RGB-D Visual Odometry, 3D Reconstruction and SLAM'
//  or: 'Intrinsic Scene Properties from a Single RGB-D Image Supplementary Material'
// The depth factor should scale depth values to meters (for instance 0.001 if depth is in mm)
// Another approach can be found here: https://github.com/shurans/SUNCGtoolbox/blob/master/gaps/pkgs/R2Shapes/R2Grid.cpp ("AddNoise")
Mat addNoise(Mat input, const PinholeParameters& intrinsics, bool withNormalShift, float depthFactor = 1, float discr = 35130.0) {

    if(input.type() != CV_32FC1) {
        cerr << "Error, wrong image format in addNoise()." << endl;
        return Mat();
    }

    const float sigma_s = 0.4f; //0.5f;
    const float sigma_d = 1.0f / 5.0f; //1.0f / 6.0f;
    GaussianNoise noiseGeneratorD(0,sigma_d);
    GaussianNoise noiseGeneratorS(0,sigma_s);

    Mat depthCM;
    input.convertTo(depthCM, CV_32FC1, 100 * depthFactor); // convert to cm

    Projected3DCloud cloud;
    if(withNormalShift) cloud.fromMat(depthCM, Mat(), intrinsics);

    auto Z = [&](int x, int y) -> float {
        return depthCM.at<float>(clamp(y+(int)noiseGeneratorS.generate(),0,depthCM.rows-1),
                                     clamp(x+(int)noiseGeneratorS.generate(),0,depthCM.cols-1));
    };

    Mat result(depthCM.rows, depthCM.cols, CV_32FC1);
    for (int i = 0; i < depthCM.rows; ++i){
        float* pOut = result.ptr<float>(i);
        for (int j = 0; j < depthCM.cols; ++j){

            pOut[j] = discr / round(discr / Z(j,i) + noiseGeneratorD.generate() + 0.5);

            if(withNormalShift){
                Point3D& point = cloud.at(j,i);
                Vec3f v = normalize(point.p);
                float angle = acos(fabs(v.dot(point.n)));

                GaussianNoise noiseGeneratorN(0, 0.3 * angle/(pOut[j]+0.1));
                float n = noiseGeneratorN.generate();

                if(angle < 0.46*M_PI) pOut[j] += n;
                else if(randomCoin((angle-M_PI_4) / M_PI_4)) pOut[j] = 0;
            }
        }
    }

    result.convertTo(result, CV_32FC1, 0.01 * depthFactor);

    // One way to visualise (or simply run CoFusion):
    // Projected3DCloud plyCloudTest(result, Mat(), intrinsics, 0, 100);
    // plyCloudTest.toPly("/path/test.ply");

    return result;
}

int main(int argc, char * argv[])
{
    Parser::init(argc, argv);

    if(!Parser::hasOption("--dir") ||
       !Parser::hasOption("--outdir") ||
       ((Parser::hasOption("-ns") ||
         Parser::hasOption("-z")) &&
         (!Parser::hasOption("-cx") ||
          !Parser::hasOption("-cy") ||
          !Parser::hasOption("-fx") ||
          !Parser::hasOption("-fy"))) ||
       (!Parser::hasOption("-n") &&
        !Parser::hasOption("-z"))){
        cout << "Error, invalid arguments.\n"
                "Mandatory --dir: Path to directory containing *.exr images.\n"
                "Mandatory --outdir: Output path.\n"
                "Mandatory -cx: Optical center x.\n"
                "Mandatory -cy: Optical center y.\n"
                "Mandatory -fx: Focal length x.\n"
                "Mandatory -fy: Focal length y.\n"
                "Mandatory, one of the following:\n"
                "Optional -n: Add noise to depth data. See 'A Benchmark for RGB-D Visual Odometry, 3D Reconstruction and SLAM'\n"
                "Optional   -ns: Include normal shifts in noise (Not perfect!).\n"
                "Optional   -dv: Discretisation value. Default: 35130.0f\n"
                "Optional -z: Apply 'distance -> z' conversion.\n"
                "\n"
                "Example: ./convert_depth --dir /path/to/image_folder/ -fx 528 -fy 528 -cx 320 -cy 240 -n"
                "\n\n"
                "Useful commands: 'See source-code of this line (comments)'" << endl;
        // Backup: rename 's/Depth(\d{4})\.exr/DB_$1/' *.exr
        // Rename converted: rename 's/-converted\.exr/\.exr/' *.exr

        return 1;
    }

    string directory = Parser::getPathOption("--dir");
    string out_directory = Parser::getPathOption("--outdir");
    bool doNoise = Parser::hasOption("-n");
    bool doConversion = Parser::hasOption("-z");
    bool doNormalShift = Parser::hasOption("-ns");
    float discrVal = Parser::getFloatOption("-dv", 35130.0f);
    bool verbose = Parser::hasOption("-v");

    PinholeParameters intrinsics;
    intrinsics.cx = Parser::getFloatOption("-cx");
    intrinsics.cy = Parser::getFloatOption("-cy");
    intrinsics.fy = Parser::getFloatOption("-fy");
    intrinsics.fx = Parser::getFloatOption("-fx");

    if(doConversion) cout << "Converting depth values..." << endl;
    if(doNoise) cout << "Adding noise..." << endl;

    vector<pair<string,string> > files; // path in / path out
    const string fileExtension = ".exr";

    using namespace boost::filesystem;
    using namespace boost::algorithm;
    for(auto it = directory_iterator(directory); it != directory_iterator(); ++it ){
        if (is_regular_file(it->status())){
            auto path = it->path();
            string ext = path.extension().string();
            to_lower(ext);
            if(fileExtension == ext)
                files.push_back(make_pair(path.string(),
                                          out_directory + path.filename().string())); // CHECK
        }
    }

    size_t numErrors = 0;
    float progressStep = 1.0 / (files.size()+1);
    unsigned i = 0;

    for(auto&& file : files){
        if(verbose) cout << "\nConverting file:\n" << file.first << " to\n" << file.second << endl;
        else showProgress(progressStep*i++);
        Mat image = imread(file.first, cv::IMREAD_UNCHANGED);

        Mat imageOut = doConversion ? convertDepth(image, intrinsics) : image;
        if(doNoise) imageOut = addNoise(imageOut, intrinsics, doNormalShift, 1.0, discrVal);

        if(imageOut.total() == 0) {
            numErrors++;
            cerr << "Conversion returned empty file." << endl;
        } else if(exists(file.second)) {
            numErrors++;
            cerr << "File exists already." << endl;
        } else {
            imwrite(file.second, imageOut);
        }
    }

    cout << "\nDone. Errors: " << numErrors << endl;

    return 0;
}
