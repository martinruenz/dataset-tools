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
#include <opencv2/calib3d/calib3d.hpp>
#include <fstream>
#include <opencv2/core/eigen.hpp>

using namespace std;
using namespace cv;

vector<Point3f> getObjectPattern(Size pattern_size, float square_size){
    vector<Point3f> result;
    for(int i = 0; i < pattern_size.height; i++ )
        for(int j = 0; j < pattern_size.width; j++ )
            result.push_back(Point3f(j*square_size,i*square_size, 0));
    return result;
}

int main(int argc, char * argv[])
{
    Parser parser(argc, argv);

    if(!parser.hasOption("-i") || !parser.hasOption("-w") || !parser.hasOption("-h")){
        cout << "Error, invalid arguments.\n"
                "Mandatory -i: Path to directory containing containing rgb images with patterns (and depth images, if -d is not used).\n"
                "Mandatory -w: Number of inner corners, axis 1.\n"
                "Mandatory -h: Number of inner corners, axis 2.\n"
                "Mandatory -s: Size of squares in mm.\n"
                "Optional -cx: Optical center x (Otherwise calibrated).\n"
                "Optional -cy: Optical center y (Otherwise calibrated).\n"
                "Optional -fx: Focal length x (Otherwise calibrated).\n"
                "Optional -fy: Focal length y (Otherwise calibrated).\n"
                "Optional -d: Path to directory containing depth images (== '-i', if not set).\n"
                "Optional -o: Optional path to output trajectory.\n"
                "Optional --prefix_d: Prefix of depth files (default: 'Depth').\n"
                "Optional --prefix_rgb: Prefix of rgb files (default: 'Color').\n"
                "Optional --depth_scale: Depth values are scaled with this factor (default: 0.2).\n"
                "Optional -v: Verbose -- print rgbd - d file associations.\n";
        return 1;
    }

    const size_t num_images_calibration = 50;
    string in_rgb_path = parser.getDirOption("-i");
    string in_depth_path = parser.hasOption("-d") ? parser.getDirOption("-d") : in_rgb_path;
    string out_path = parser.getDirOption("-o");
    string prefix_rgb = parser.getStringOption("--prefix_rgb", "Color");
    string prefix_d = parser.getStringOption("--prefix_d", "Depth");
    Size pattern_size(parser.getIntOption("-h"), parser.getIntOption("-w"));
    Size image_size(0,0);
    float square_size = parser.getFloatOption("-s");
    float depth_scale = parser.getFloatOption("depth_scale", 0.2);
    bool verbose = parser.hasOption("-v");
    bool do_export = parser.hasOption("-o");
    bool has_calibration = parser.hasOption("-cx") && parser.hasOption("-cy") && parser.hasOption("-fx") && parser.hasOption("-fy");

    if(!exists(in_rgb_path) || !exists(in_depth_path) || (do_export && !exists(out_path))) {
        cerr << "Directory does not exist." << endl;
        return 1;
    }

    if(verbose){
        cout << "Using pattern with w: " << pattern_size.width << ", and h: " << pattern_size.height << endl;
    }

    std::vector<std::pair<std::string,std::string>> files = getFilePairs(in_rgb_path,in_depth_path,prefix_rgb,prefix_d,{".jpg", ".png"},{".png"});
    if(!validateMatchingIndexes(files)) cerr << "Finding matching files failed (remove the line causing this error, if files don't exhibit indexes." << endl;

    Mat calibration;
    if(has_calibration){
        double data[9] = { parser.getDoubleOption("-fx"), 0, parser.getDoubleOption("-cx"), 0, parser.getDoubleOption("-fy"), parser.getDoubleOption("-cy"), 0, 0, 1 };
        Mat(3, 3, CV_64F, data).copyTo(calibration);
    } else {
        calibration = Mat::eye(3, 3, CV_64F);
    }
    Mat dist_coeffs = Mat::zeros(8, 1, CV_64F);
    vector<Mat> roration_vectors, translation_vectors;
    vector<vector<Point2f>> image_points;
    vector<vector<Point3f>> object_points;
    vector<Point3f> object_pattern = getObjectPattern(pattern_size, square_size);
    vector<float> depth_values;
    vector<string> files_pattern_found;

    image_points.reserve(files.size());
    object_points.reserve(files.size());
    depth_values.reserve(files.size());
    files_pattern_found.reserve(files.size());

    size_t progress = 0;
    float progressStep = 1.0 / (files.size()+1);
    unsigned cnt_pattern_found = 0;

    // Find chessboards
    cout << "Searching chessboards ..." << endl;
    for(const auto& pair : files){

        // Process rgb data
        cv::waitKey(1);
        showProgress(++progress*progressStep);

        vector<Point2f> current_corners;
        Mat rgb = imread(in_rgb_path + pair.first);
        Mat grey;
        cvtColor(rgb, grey, COLOR_BGR2GRAY);
        if(image_size.area() > 0){
            if(image_size != rgb.size()) {
                cerr << "All image dimension have to match!" << endl;
                return 1;
            }
        } else {
            image_size = rgb.size();
        }

        bool found_pattern = findChessboardCorners(rgb, pattern_size, current_corners, CALIB_CB_ADAPTIVE_THRESH | CALIB_CB_NORMALIZE_IMAGE); // CALIB_CB_FAST_CHECK
        if(!found_pattern) {
            cerr << "Warning, could not find pattern in image: " << pair.first << endl;
            imshow("view", rgb);
            continue;
        }
        if(verbose) {
            cout << "Found pattern in image: " << pair.first << endl;
        }

        cnt_pattern_found++;
        drawChessboardCorners(rgb, pattern_size, Mat(current_corners), true);
        imshow("view", rgb);

        cornerSubPix(grey, current_corners, Size(11,11), Size(-1,-1), TermCriteria(TermCriteria::EPS+TermCriteria::COUNT, 30, 0.1));
        image_points.push_back(current_corners);
        object_points.push_back(object_pattern);

        // Read depth data
        Mat depth = imread(in_depth_path + pair.second, IMREAD_UNCHANGED);
        if(depth.type() != CV_16UC1) {
            cerr << "Invalid depth image: " << pair.second << endl;
            return 1;
        }
        depth_values.push_back(depth_scale * depth.at<ushort>(current_corners[0]));
        files_pattern_found.push_back(getBasename(pair.first));

        // REMOVE!
        //if(cnt_pattern_found > 120) break;
    }
    cout << endl;

    // Calibrate
    if(!has_calibration){
        cout << "Starting calibration ..." << endl;
        vector<vector<Point2f>> cal_image_points;
        vector<vector<Point3f>> cal_object_points;
        if(cnt_pattern_found <= num_images_calibration) {
            cal_image_points = image_points;
            cal_object_points = object_points;
        } else {
            cout << "Using subset of " << num_images_calibration << " images for calibration ..." << endl;
            cout << "Warning! If you use video data, images are most likely not suited for calibration!" << endl;
            cal_image_points.reserve(num_images_calibration);
            cal_object_points.reserve(num_images_calibration);
            size_t step = cnt_pattern_found / num_images_calibration;
            for(size_t i = 0; i < num_images_calibration; i++){
                const size_t index = step * i;
                cal_image_points.push_back(image_points[index]);
                cal_object_points.push_back(object_points[index]);
            }
        }

        double rms = calibrateCamera(cal_object_points, cal_image_points, image_size,
                                     calibration, dist_coeffs,
                                     roration_vectors, translation_vectors,
                                     CALIB_FIX_K3|CALIB_FIX_K4|CALIB_FIX_K5);
        cout << "Final re-projection error: " << rms << "\n";
        cout << "Calibration matrix: \n" << calibration << endl;
    } else {
        cout << "Skipping calibration ..." << endl;
        cout << "Calibration matrix: \n" << calibration << endl;
    }

    // Compute poses based on pattern (PnP)
    roration_vectors.clear();
    translation_vectors.clear();
    progress = 0;
    progressStep = 1.0 / (image_points.size()+1);
    for(size_t i = 0; i < image_points.size(); i++){
        showProgress(++progress*progressStep);
        roration_vectors.push_back(cv::Mat());
        translation_vectors.push_back(cv::Mat());
        solvePnP(object_pattern, image_points[i], calibration, dist_coeffs, roration_vectors.back(), translation_vectors.back());
    }


    if(translation_vectors.size() != depth_values.size()){
        cerr << "depth_values and translation_vectors do not match." << endl;
        return 1;
    }

    //ofstream file(out_path, ofstream::out);
    std::ofstream file_poses;
    if(do_export) file_poses.open(out_path+"/poses.txt");

    double avg_d = 0, avg_z = 0;
    size_t cnt = 0;
    // Compare depth values
    for(size_t i=0; i<depth_values.size(); i++){
        double d = depth_values[i];
        Mat t;
        translation_vectors[i].copyTo(t);
        double z = t.at<double>(2);

        if(verbose) cout << "tz: " << z << " d: " << d << "\n";
        if(depth_values[i] > 0){
            avg_d += d;
            avg_z += z;
            cnt++;
        }

        if(do_export) {
            // Extract quaternion
            Mat R;
            Rodrigues(roration_vectors[i], R);
            //            assert(R.type() == CV_64FC1);
            //            assert((R.cols == 3) && (R.rows == 3));
            //            Eigen::Map<Eigen::Matrix3d> rot_eigen(R.data());
            //

            Eigen::Matrix3d rot_eigen;
            cv2eigen(R,rot_eigen);
            Eigen::Quaterniond q(rot_eigen);

            const string sep = "\t";
            t *= 0.001;
            file_poses << files_pattern_found[i] << sep << t.at<double>(0) << sep << t.at<double>(1) << sep << t.at<double>(2)
                       << q.x() << sep << q.y() << sep << q.z() << sep << q.w() << "\n";
        }
    }
    avg_z /= cnt;
    avg_d /= cnt;
    cout << "Avg. z-values: " << avg_z << "mm, avg. d-values: " << avg_d << "mm (" << 100 * avg_z / avg_d << "%)" << endl;
    cout << "Found pattern in " << 100.0f * cnt_pattern_found / files.size() << "% of the images." << endl;

    if(do_export) file_poses.close();

    return 0;
}
