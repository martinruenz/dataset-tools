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

/*
 * TODOs:
 * - This program is not working nicely with 16bit uint images (enable 16 bit input / output)
 */

#include "../common/common.h"
#include "../common/common_labels.h"
#include "../common/common_image.h"

using namespace std;
using namespace cv;

// The returned pair holds the result (input after extract/replace) and a masked version of the input, containing only the searched colors
pair<Mat,Mat> findReplaceColor(const Mat& input,
                               const vector<Vec3b>& colors,
                               const vector<Vec3b>& replace_colors,
                               bool extract){

    if(input.type() != CV_8UC3) { // This requirement could easily be avoided
        throw std::invalid_argument("Error, wrong image format");
        return {Mat(),Mat()};
    }

    bool do_replace = (replace_colors.size() == colors.size());
    bool write_out = true;
    Mat result;
    Mat input_masked(input.rows, input.cols, CV_8UC3);
    if(extract) {
        result = Mat::zeros(input.rows, input.cols, input.type()); //Mat(input.rows, input.cols, input.type(), Vec3b(0,0,0)).copyTo(*out);
    } else {
        input.copyTo(result);
        if(!replace_colors.size()) write_out = false;
    }

    // Stencil color in input and create output image
    for (size_t i = 0; i < input.total(); ++i){
        const Vec3b& pixel = input.at<Vec3b>(i);
        auto find_result = std::find(colors.begin(), colors.end(), pixel);
        if(find_result == colors.end()) input_masked.at<Vec3b>(i) = Vec3b(0,0,0);
        else {
            input_masked.at<Vec3b>(i) = pixel;
            if(write_out){
                Vec3b& pixel_out = result.at<Vec3b>(i);
                if(do_replace) pixel_out = replace_colors[std::distance(colors.begin(), find_result)];
                else pixel_out = pixel;
            }
        }
    }

    return {result, input_masked};
}

Mat createLabelOverlay(const Mat masked_input, const Mat background){
    if(!background.total()) return masked_input;
    if(background.type() != CV_8UC3 ||
            background.cols != masked_input.cols ||
            background.rows != masked_input.rows)
        throw std::invalid_argument("Error, wrong reference image format");

    Mat result(masked_input.rows, masked_input.cols, CV_8UC3);
    for (size_t i = 0; i < masked_input.total(); ++i)
        result.at<Vec3b>(i) = 0.85 * masked_input.at<Vec3b>(i) + 0.15 * background.at<Vec3b>(i);
    return result;
}

int main(int argc, char * argv[])
{
    Parser parser(argc, argv);

    if(!parser.hasOption("--dir")
            || (!parser.hasOption("-c") && !parser.hasOption("-s"))){
        cout << "This tool allows you to find or replace single labels in a dataset.\n\n";
        cout << "Error, invalid arguments.\n"
                "Mandatory -c: RGB-color(s) that is searched, eg 200,100,180 190,50,110. This or '-s' is required.\n"
                "Mandatory -s: Summarise, creates a text-file of which each line lists the labels (!=0) in an image. This or '-c' is required.\n"
                "Mandatory --dir: Path to directory containing label images.\n"
                "Optional --outdir: Output directory\n"
                "Optional --ref: Path to directory containing #####.png images reference images, that are simply displayed with --dir images.\n"
                "Optional -r: Replace color(s), and write to --outdir. Must have same number of colors as '-c'. eg 0,0,0 1,1,1\n"
                "Optional -e: Extract colors. All colors not specified with '-c' will be removed from the images\n"
                "Optional --id_image: If set, improves the visualisation if id-images, also changes output of '-s'.\n"
                "\n"
                "Example: ./label_finder --dir /path/to/mask_folder/ --ref /path/to/color_folder/ -c 200,100,180\n"
                "Example: ./label_finder --dir /path/to/mask_folder/ --outdir /path/to/id_mask_folder [--ref /path/to/color_folder/] -c 200,100,180 -r 1,1,1\n" << endl;

        return 1;
    }

    bool id_image = parser.hasOption("--id_image");
    string directory = parser.getDirOption("--dir");
    string ref_directory = parser.getDirOption("--ref");
    string out_directory = parser.getDirOption("--outdir");
    vector<Vec3b> colors = stringToColors(parser.getOption("-c"), true);
    vector<Vec3b> replacement_colors = stringToColors(parser.getOption("-r"), true);
    bool extract_colors = parser.hasOption("-e");
    bool replace_colors = replacement_colors.size() > 0;
    bool write_output = (replace_colors || extract_colors);
    bool create_label_summary = parser.hasOption("-s");

    if(parser.hasOption("--outdir") && !write_output && !create_label_summary) throw std::invalid_argument("Error, invalid arguments");
    if(replace_colors && replacement_colors.size() != colors.size()) throw std::invalid_argument("Error, invalid arguments");

    vector<string> files = getFilenames(directory, {".png", ".pgm", ".ppm"});

    std::ofstream summary_stream;
    if(create_label_summary){
        summary_stream.open(out_directory+"labels.txt");
    }

    for(auto&& file : files){
        string path_input = directory + file;
        string path_output = out_directory + file;
        string path_ref = ref_directory + getBasename(file) + ".png";

        Mat image = imread(path_input, cv::IMREAD_UNCHANGED);
        std::cout << "sum x " << cv::sum(image)[0] << std::endl;
        //if(greyscale) cvtColor(image,image, CV_GRAY2RGB);
        if(image.type() == CV_8UC1) merge(vector<Mat>{image,image,image},image);
        pair<Mat, Mat> output = findReplaceColor(image, colors, replacement_colors, extract_colors);
        imshow("find_color", createLabelOverlay(id_image ? labelToColourImage(output.second) : output.second,
                                                ref_directory.length() ? imread(path_ref) : Mat()));

        std::cout << "sum 1 " << cv::sum(output.first)[0] << std::endl;
        if(write_output) {
            imshow("out_color", output.first);
            //if(greyscale)
//                cvtColor(output.first, output.first, CV_RGB2GRAY); // TODO splitting would be more efficient
                std::vector<Mat> out;
                split(output.first, out);
                out[0].convertTo(out[0], CV_16UC1);

                std::cout << "sum 2 " << cv::sum(out[0])[0] << std::endl;
                std::cout << cvTypeToString(out[0].type()) << std::endl;
                std::cout << "writing: " << path_output << std::endl;
            if(out_directory.length()) {
                imwrite(path_output, out[0]);
            }
        }
        if(create_label_summary){
            vector<Vec3b> image_colors;
            findColors(output.first, image_colors);
            for (int i = 0; i < int(image_colors.size())-1; ++i) {
                summary_stream << colorToString(image_colors[i], id_image) << " ";
            }
            if (image_colors.size() > 0) summary_stream << colorToString(image_colors[image_colors.size()-1], id_image);
            summary_stream << "\n";
            //imshow("labels", id_image ? labelToColourImage(output.first) : output.first);
        }
        waitKey(1);
    }

    if(create_label_summary) summary_stream.close();

    return 0;
}
