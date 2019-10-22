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

int main(int argc, char * argv[])
{
    Parser parser(argc, argv);

    if(!parser.hasOption("--dir") || !parser.hasOption("--outdir")){
        cout << "This tool converts RGB-images to ID-images, or vice-versa.\n\n";
        cout << "Error, invalid arguments.\n"
                "Mandatory --dir: Path to directory containing mask images.\n"
                "Mandatory --outdir: Path to output directory.\n"
                "Optional -p: Store .png instead of .pgm files. (Implicit if --toRGB)\n"
                "Optional -s: Skip existing files.\n"
                "Optional -n: Just simulate and don't write anything to disc.\n"
                "Optional -v: Verbose.\n"
                "Optional --toRGB: Convert to ID to RGB.\n"
                "\n"
                "Example: ./convert_masks --dir /path/to/input_folder --outdir /path/to/out_folder/\n"
                "\n\n"
                "Useful commands: 'See source-code of this line (comments)'" << endl;
                // Backup: rename 's/Mask(\d{4})\.png/MB_$1/' *.png
                // Rename converted: rename 's/-converted\.png/\.png/' *.png

        return 1;
    }

    string directory = parser.getDirOption("--dir");
    string out_directory = parser.getDirOption("--outdir");
    bool skipExisting = parser.hasOption("-s");
    bool doWrite = !parser.hasOption("-n");
    bool verbose = parser.hasOption("-v");
    bool toRGB = parser.hasOption("--toRGB");
    bool storePNG = parser.hasOption("-p") || toRGB;

    vector<string> files = getFilenames(directory, {".png", ".ppm"});

    std::vector<cv::Vec3b> colors;
    size_t numErrors = 0;

    for(auto&& file : files){
        string path_input = directory + file;
        string path_output = out_directory + getBasename(file) + (storePNG ? ".png" : ".pgm");
        if(skipExisting && exists(path_output)){
            cout << "Skipping file: " << path_output << " (already exists). Warning: Completely ignored!" << endl;
            continue;
        }
        if(verbose) cout << "\nConverting file:\n" << path_input << " to\n" << path_output << endl;
        Mat image = imread(path_input);
        Mat image_out;
        if(toRGB) image_out = labelToColourImage(image);
        else image_out = colourToLabelImage(image, colors);
        if(image_out.total() == 0) numErrors++;
        else if(doWrite) {
            if(storePNG) imwrite(path_output, image_out);
            else imwrite(path_output, image_out, { cv::IMWRITE_PXM_BINARY });
        }
    }

    cout << "\nDone. Errors: " << numErrors;
    if(!toRGB) cout << " Number of masks: " << colors.size();
    cout << endl;

    return 0;
}
