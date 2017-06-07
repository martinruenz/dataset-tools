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

using namespace std;
using namespace cv;

// Overwrite input image with visualisation, out is overwritten
void showColor(Mat& input, const Vec3b& color, const Mat& reference = Mat(), Mat* out = 0, const Vec3b* replace_color = 0){

    if(input.type() != CV_8UC3) { // This requirement could easily be avoided
        throw std::invalid_argument("Error, wrong image format");
        //return;
    }

    if(replace_color)
        input.copyTo(*out);

    // Stencil color in input and create output image
    for (int i = 0; i < input.rows; ++i){
        for (int j = 0; j < input.cols; ++j){
            Vec3b& pixel = input.at<Vec3b>(i, j);
            if(pixel != color) pixel = Vec3b(0,0,0);
            else if(replace_color) {
                Vec3b& pixel_out = out->at<Vec3b>(i, j);
                pixel_out = *replace_color;
            }
        }
    }

    if(reference.total()){
        if(reference.type() != CV_8UC3 || reference.cols != input.cols || reference.rows != input.rows)
            throw std::invalid_argument("Error, wrong reference image format");

        for (int i = 0; i < input.rows; ++i){
            for (int j = 0; j < input.cols; ++j){
                Vec3b& pixel = input.at<Vec3b>(i, j);
                const Vec3b& pixel_ref = reference.at<Vec3b>(i, j);
                pixel = 0.85 * pixel + 0.15 * pixel_ref;
            }
        }
    }
}

int main(int argc, char * argv[])
{
    Parser::init(argc, argv);

    if(!Parser::hasOption("--dir") || !Parser::hasOption("-c")){
        cout << "This tool allows you to find or replace single labels in a dataset.\n\n";
        cout << "Error, invalid arguments.\n"
                "Mandatory -c: RGB-color that is searched, eg 200,100,180\n"
                "Mandatory --dir: Path to directory containing #####.png images that are to be searched.\n"
                "Optional --outdir: Output directory\n"
                "Optional --ref: Path to directory containing #####.png images reference images, that are simply displayed with --dir images.\n"
                "Optional -r: Replace color, and write to --outdir, eg 0,0,0\n"
                "Optional --greyscale: Has to be set when working with 1-channel images\n"
                "Optional --starti: start-index\n"
                "Optional --width: your index-width\n"
                "Optional --prefix: filename prefix\n"
                "\n"
                "Example: ./label_finder --dir /path/to/mask_folder/ --ref /path/to/color_folder/ -c 200,100,180\n"
                "Example: ./label_finder --dir /path/to/mask_folder/ --outdir /path/to/id_mask_folder --prefix Mask --width 4 --starti 1 [--ref /path/to/color_folder/] -c 200,100,180 -r 1,1,1\n" << endl;

        return 1;
    }

    bool greyscale = Parser::hasOption("--greyscale");
    string directory = Parser::getPathOption("--dir");
    string ref_directory, out_directory;
    Vec3b color = greyscale ? Vec3b(Parser::getIntOption("-c"),Parser::getIntOption("-c"),Parser::getIntOption("-c")) :
                              stringToColor(Parser::getOption("-c"));
    Vec3b replace_color;
    Vec3b* r_color = 0; // Only set if provided

    int index_width = Parser::getIntOption("--width", 5);
    int start_index = Parser::getIntOption("--starti");
    string prefix = Parser::getOption("--prefix");

    if(Parser::hasOption("--ref"))
        ref_directory = Parser::getPathOption("--ref");

    if(Parser::hasOption("-r")) {
        replace_color = greyscale ? Vec3b(Parser::getIntOption("-r"),Parser::getIntOption("-r"),Parser::getIntOption("-r")) :
                                    stringToColor(Parser::getOption("-r"));
        r_color = &replace_color;
    }
    if(Parser::hasOption("--outdir")){
        if(r_color==0) throw std::invalid_argument("Error, invalid arguments");
        out_directory = Parser::getPathOption("--outdir");
    }


    for(int currentFrame=start_index; ; currentFrame++){

        stringstream ss;
        ss << setw(index_width) << setfill('0') << currentFrame;
        std::string indexStr = ss.str();

        //cout << "Reading frame " << indexStr << endl;

        string impath = directory + prefix + indexStr + ".png";
        string refpath = ref_directory + indexStr + ".png";
        if(!exists(impath)) break;

        Mat image = imread(impath);
        //if(greyscale) cvtColor(image,image, CV_GRAY2RGB);
        Mat out_image;
        showColor(image, color, ref_directory.length() ? imread(refpath) : Mat(), &out_image, r_color);
        if(greyscale) image = labelToColourImage(image);
        imshow("find_color", image);
        if(r_color) {
            imshow("out_color", out_image);
            if(greyscale) cvtColor(out_image,out_image, CV_RGB2GRAY);
            if(out_directory.length()) imwrite(out_directory + prefix + indexStr + ".png", out_image);
        }
        waitKey(1);
    }

    return 0;
}
