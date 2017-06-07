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
    Parser::init(argc, argv);

    if(!Parser::hasOption("-d")){
        cout << "This tool converts RGB-images to ID-images, or vice-versa.\n\n";
        cout << "Error, invalid arguments.\n"
                "Mandatory -d: Path to directory containing Mask####.png images.\n"
                "Optional -p: Store *-converted.png instead of *.pgm files. (Implicit if --toRGB)\n"
                "Optional -s: Skip existing files.\n"
                "Optional -a: Treat all png files (not just the ones starting with 'Mask..') as input.\n"
                "Optional -n: Just simulate and don't write anything to disc.\n"
                "Optional -v: Verbose.\n"
                "Optional --toRGB: Convert to ID to RGB.\n"
                "\n"
                "Example: ./convert_masks -d /path/to/image_folder/\n"
                "\n\n"
                "Useful commands: 'See source-code of this line (comments)'" << endl;
                // Backup: rename 's/Mask(\d{4})\.png/MB_$1/' *.png
                // Rename converted: rename 's/-converted\.png/\.png/' *.png

        return 1;
    }

    string directory = Parser::getOption("-d");
    bool skipExisting = Parser::hasOption("-s");
    bool allPNGs = Parser::hasOption("-a");
    bool doWrite = !Parser::hasOption("-n");
    bool verbose = Parser::hasOption("-v");
    bool toRGB = Parser::hasOption("--toRGB");
    bool storePNG = Parser::hasOption("-p") || toRGB;

    vector<pair<string,string> > files; // path in / path out

    using namespace boost::filesystem;
    using namespace boost::algorithm;
    for(auto it = directory_iterator(directory); it != directory_iterator(); ++it ){
        if (is_regular_file(it->status())){
            auto path = it->path();
            string name = path.stem().string();
            string ext = path.extension().string();
            to_lower(ext);
            if((name.substr(0,4) == "Mask" || allPNGs) && ext == ".png" && name.find("converted") == std::string::npos) {
                if(storePNG) files.push_back(make_pair(path.string(), path.parent_path().string() + "/" + name + "-converted.png"));
                else files.push_back(make_pair(path.string(), path.parent_path().string() + "/" + name + ".pgm"));
            }
        }
    }

    std::vector<cv::Vec3b> colors;
    size_t numErrors = 0;

    for(auto&& file : files){
        if(skipExisting && exists(file.second)){
            cout << "Skipping file: " << file.second << " (already exists). Warning: Completely ignored!" << endl;
            continue;
        }
        if(verbose) cout << "\nConverting file:\n" << file.first << " to\n" << file.second << endl;
        Mat image = imread(file.first);
        Mat imageOut;
        if(toRGB) imageOut = labelToColourImage(image);
        else imageOut = colourToLabelImage(image, colors);
        if(imageOut.total() == 0) numErrors++;
        else if(doWrite) {
            if(storePNG) imwrite(file.second, imageOut);
            else imwrite(file.second, imageOut, { CV_IMWRITE_PXM_BINARY });
        }
    }

    cout << "\nDone. Errors: " << numErrors;
    if(!toRGB) cout << " Number of masks: " << colors.size();
    cout << endl;

    return 0;
}
