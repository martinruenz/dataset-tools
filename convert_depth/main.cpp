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
#include "../common/common_random.h"
#include "../common/common_3d.h"

using namespace std;
using namespace cv;

// Input: Projective depth values (distances to camera center)
// Output: Depth values (z-component)
Mat convertDistanceToZ(Mat input, const PinholeParameters& intrinsics){

    if(input.type() != CV_32FC3)
    {
        cerr << "Error, wrong image format in convertDistanceToZ()." << endl;
        return Mat();
    }

    PinholeCamera cam(intrinsics);

    Mat result(input.rows, input.cols, CV_32FC1);

    for (int i = 0; i < input.rows; ++i){
        cv::Vec3f* pixel = input.ptr<cv::Vec3f>(i);
        float* pOut = result.ptr<float>(i);
        for (int j = 0; j < input.cols; ++j){
            Eigen::Vector3d ray = cam.toRay(Eigen::Vector2d(j,i));
            ray *= pixel[j][0];
            pOut[j] = ray[2];
        }
    }
    return result;
}

// Input: Disparity values
// Output: Depth values (z-component)
// As used in https://github.com/victorprad/InfiniTAM/blob/1f52e8c85df795cb7643977059b88c81e4a56e40/InfiniTAM/ITMLib/Engines/ViewBuilding/Shared/ITMViewBuilder_Shared.h
Mat convertDisparityToZ(Mat input){

    if(input.type() != CV_16UC1)
    {
        cerr << "Error, wrong image format in convertDisparityToZ()." << endl;
        return Mat();
    }

    Mat result(input.rows, input.cols, CV_32FC1);
    float a = 1135.09;
    float b = 1135.09;
    float fx_depth = 573.71;
    for (int i = 0; i < input.rows; ++i){
        unsigned short* pixel = input.ptr<unsigned short>(i);
        float* pOut = result.ptr<float>(i);
        for (int j = 0; j < input.cols; ++j){
            float disparity_tmp = (a - (float)(pixel[j]));
            (disparity_tmp == 0) ? pOut[j] = 0.0 : pOut[j] = 0.001f * b * fx_depth / disparity_tmp;
            if(pOut[j] < 0) pOut[j] = 0;
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
                Eigen::Vector3d v = point.p.normalized();
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
    Parser parser(argc, argv);

    if(!parser.hasOption("--dir") ||
       !parser.hasOption("--outdir") ||
       ((parser.hasOption("-ns") ||
         parser.hasOption("-z")) &&
         (!parser.hasOption("-cx") ||
          !parser.hasOption("-cy") ||
          !parser.hasOption("-fx") ||
          !parser.hasOption("-fy"))) ||
       (!parser.hasOption("-n") &&
        !parser.hasOption("-z") &&
        !parser.hasOption("-d"))){
        cout << "Error, invalid arguments.\n"
                "Mandatory --dir: Path to directory containing depth images.\n"
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
                "Optional -d: Apply 'disparity -> z' conversion.\n"
                "\n"
                "Example: ./convert_depth --dir /path/to/image_folder/ -fx 528 -fy 528 -cx 320 -cy 240 -n"
                "\n\n"
                "Useful commands: 'See source-code of this line (comments)'" << endl;
        // Backup: rename 's/Depth(\d{4})\.exr/DB_$1/' *.exr
        // Rename converted: rename 's/-converted\.exr/\.exr/' *.exr

        return 1;
    }

    string directory = parser.getDirOption("--dir");
    string out_directory = parser.getDirOption("--outdir");
    bool doNoise = parser.hasOption("-n");
    bool do_distConversion = parser.hasOption("-z");
    bool do_dispConversion = parser.hasOption("-d");
    bool do_normalShift = parser.hasOption("-ns");
    float discr_val = parser.getFloatOption("-dv", 35130.0f);
    bool verbose = parser.hasOption("-v");

    PinholeParameters intrinsics;
    intrinsics.cx = parser.getFloatOption("-cx");
    intrinsics.cy = parser.getFloatOption("-cy");
    intrinsics.fy = parser.getFloatOption("-fy");
    intrinsics.fx = parser.getFloatOption("-fx");

    if(do_distConversion) cout << "Converting dist->depth values..." << endl;
    else if(do_dispConversion) cout << "Converting disp->depth values..." << endl;
    if(doNoise) cout << "Adding noise..." << endl;

    vector<string> files = getFilenames(directory, {".exr", ".pgm"});

    size_t num_errors = 0;
    float progressStep = 1.0 / (files.size()+1);
    unsigned i = 0;

    for(auto&& file : files){
        string path_input = directory + file;
        string path_output = out_directory + getBasename(file) + ".exr";
        if(verbose) cout << "\nConverting file:\n" << path_input << " to\n" << path_output << endl;
        else showProgress(progressStep*i++);
        Mat image = imread(path_input, cv::IMREAD_UNCHANGED);
        Mat image_out = image;

        // apply conversions
        if(do_distConversion) image_out = convertDistanceToZ(image, intrinsics);
        else if(do_dispConversion) image_out = convertDisparityToZ(image);
        if(doNoise) image_out = addNoise(image_out, intrinsics, do_normalShift, 1.0, discr_val);

        if(image_out.total() == 0) {
            num_errors++;
            cerr << "Conversion returned empty file." << endl;
        } else if(exists(path_output)) {
            num_errors++;
            cerr << "File exists already." << endl;
        } else {
            imwrite(path_output, image_out);
        }
    }

    cout << "\nDone. Errors: " << num_errors << endl;

    return 0;
}
