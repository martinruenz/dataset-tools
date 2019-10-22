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

using namespace boost::filesystem;
using namespace std;
using namespace cv;

Mat mergeChannels(Mat input){

    if(input.type() != CV_32FC3) {
//        cerr << "Assuming 3 input channels." << std::endl;
//        return Mat();
        throw invalid_argument("Assuming 3 input channels.");
    }

    Mat result(input.size(), CV_32FC1);

    for (int i = 0; i < input.rows; ++i){
        Vec3f* row_in = input.ptr<Vec3f>(i);
        float* row_out = result.ptr<float>(i);
        for (int j = 0; j < input.cols; ++j){
            Vec3f& pixel_in = row_in[j];
            float& pixel_out = row_out[j];
            if(pixel_in[0] != pixel_in[1] || pixel_in[0] != pixel_in[2]) cout << "Values of channels differ" << endl;
            pixel_out = float(pixel_in[0]); // FIXME 'merge'
        }
    }

    return result;
}

void convert(const std::string& input, const std::string& output){
    Mat merged = mergeChannels(imread(input, cv::IMREAD_UNCHANGED));
    if(!merged.empty()) {
        imwrite(output, merged);
    } else {
        cout << " Not processing file " << input << std::endl;
    }
}

int main(int argc, char * argv[])
{
    Parser parser(argc, argv);

    if(!parser.hasOption("-i")) {
        cout << "Error, invalid arguments.\n"
                "Mandatory -i: Input image or directory.\n"
                "Optional -o: Output image or directory.\n"
                "\n"
                "Example: ./merge_exr -i /path/to/input.exr -o /path/to/output.exr";

        return 1;
    }

    string input = parser.getOption("-i");
    string output = parser.getOption("-o");

    if(!exists(input)) throw invalid_argument("Input not found");
    if(output.empty()) output = input;

    if(is_directory(input)) {
        if(!is_directory(output))
            throw std::invalid_argument("Input / ouput should both be files, or both be directories.");
        std::vector<string> files = getFilenames(input, { ".exr" });
        Progress progress(files.size());
        for(string& file : files){
            convert(input + "/" + file, output + "/" + file);
            progress.show();
        }
    } else {
        convert(input, output);
    }

    return 0;
}
