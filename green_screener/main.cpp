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
#include "../common/common_labels.h"

#include <opencv2/imgproc/imgproc.hpp>

using namespace boost::filesystem;
using namespace std;
using namespace cv;

Mat extractObjectMask(Mat image, const Vec3b& bg_color, float fuzz = 0.08){

    assert(fuzz > 0 && fuzz < 1);
    assert(image.total() > 0);
    assert(image.channels() == 3);

    Mat hsv;
    Mat foreground(image.rows, image.cols, CV_8UC1);
    Vec3b bg_hsv;
    Mat_<Vec3b> bg_hsv_m;
    int max_diff = fuzz * 180;
    cvtColor(image, hsv, cv::COLOR_BGR2HSV);
    cvtColor(Mat_<Vec3b>({1,1}, bg_color), bg_hsv_m, cv::COLOR_BGR2HSV);
    bg_hsv = bg_hsv_m.at<Vec3b>(0,0);


    for (int i = 0; i < image.rows; ++i){
        cv::Vec3b* p_hsv = hsv.ptr<cv::Vec3b>(i);
        uchar* p_foreground = foreground.ptr<uchar>(i);
        for (int j = 0; j < image.cols; ++j){
            p_foreground[j] = (std::abs(bg_hsv[0] - p_hsv[j][0]) % 180 > max_diff || std::abs(bg_hsv[1] - p_hsv[j][1]) > 120) ? 255 : 0;
        }
    }
    Mat stats_components, centroids_components, labels_components;
    int num_components = cv::connectedComponentsWithStats(foreground, labels_components, stats_components, centroids_components, 4);
    imshow("components", labelToColourImage(labels_components));

    // Find largest label that does not touch the background
    int largest_valid_label = 0;
    int largest_valid_label_size = 0;
    for(int c = 1; c < num_components; c++){
        if(stats_components.at<int>(c, 0) > 0 &&
                stats_components.at<int>(c, 1) > 0 &&
                stats_components.at<int>(c, 2) + stats_components.at<int>(c, 0) < image.cols-1 &&
                stats_components.at<int>(c, 3) + stats_components.at<int>(c, 1) < image.rows-1 &&
                largest_valid_label_size < stats_components.at<int>(c, 4)
                ){
            largest_valid_label = c;
            largest_valid_label_size = stats_components.at<int>(c, 4);
        }
    }

    // Mask largest valid label only
    for (int i = 0; i < image.rows; ++i){
        int* p_label = labels_components.ptr<int>(i);
        uchar* p_foreground = foreground.ptr<uchar>(i);
        for (int j = 0; j < image.cols; ++j)
            if(p_label[j] != largest_valid_label) p_foreground[j] = 0;
    }

    // Fill holes
    foreground = imfill(foreground);

    return foreground;
}

int main(int argc, char * argv[])
{
    Parser parser(argc, argv);

    if(!parser.hasOption("-i") || !parser.hasOption("-o")) {
        cout << "Error, invalid arguments.\n"
                "Mandatory -i: Input directory.\n"
                "Mandatory -o: Output directory.\n"
                "Mandatory -c: Background color.\n"
                "\n"
                "Example: ./green_screener -i /path/to/input -o /path/to/out/masks -c 0,255,0";

        return 1;
    }

    string input = parser.getDirOption("-i", false);
    string output = parser.getDirOption("-o", false);
    Vec3b color = parser.getRgbOption("-c");

    vector<string> files = getFilenames(input, {".png", ".jpg"});

    Progress progress(files.size());
    for(auto&& file : files){

        Mat image = imread(input + file);
        Mat mask = extractObjectMask(image, color);
        imshow("Input", image);
        imshow("Mask", mask);
        imwrite(output + getBasename(file) + ".png", mask);

        progress.show();
        waitKey(80);
    }

    return 0;
}
