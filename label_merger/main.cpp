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

using namespace std;
using namespace cv;

int getColorIndex(const vector<Vec3b>& colors, const Vec3b& color){
    for(size_t i = 0; i < colors.size(); i++)
        if(color==colors[i])
            return i;
    return -1;
}

void findColors(Mat input, vector<Vec3b>& colors){

    if(input.type() != CV_8UC3) {
        throw std::invalid_argument("Error, invalid image format.");
        return;
    }

    for (int i = 0; i < input.rows; ++i){
        for (int j = 0; j < input.cols; ++j){
            Vec3b pixel = input.at<Vec3b>(i, j);
            int colorIndex = getColorIndex(colors, pixel);
            if(colorIndex < 0 && pixel != Vec3b(0,0,0)) colors.push_back(pixel);
        }
    }
}


Mat countColorNeighbors(Mat input, const vector<Vec3b>& colors){
    Mat neighborCounts = Mat::zeros(colors.size(),colors.size(),CV_32SC1);

    auto getLabelIndex = [&colors](Vec3b color) -> int {
        for (size_t i = 0; i < colors.size(); ++i)
            if(colors[i]==color)
                return i;
        assert(0);
        return -1;
    };
    vector<pair<int,int>> neighbors = {{-1, 0},{1, 0},{0, -1},{0, 1}};
    for (int i = 1; i < input.rows-1; ++i){
        for (int j = 1; j < input.cols-1; ++j){
            Vec3b pixel = input.at<Vec3b>(i, j);
            if(pixel==Vec3b(0,0,0)) continue;
            int i1 = getLabelIndex(pixel);
            for(auto& n : neighbors){
                Vec3b pn = input.at<Vec3b>(i+n.first, j+n.second);
                if(pn==Vec3b(0,0,0)) continue;
                if(pixel!=pn) {
                    int i2 = getLabelIndex(pn);
                    neighborCounts.at<int>(min(i1,i2),max(i1,i2))++;
                }
            }
        }
    }

    return neighborCounts;
}

int main(int argc, char * argv[])
{
    Parser::init(argc, argv);

    if(!Parser::hasOption("--dir") || !Parser::hasOption("--outdir")){
        cout << "Error, invalid arguments.\n"
                "Mandatory --dir: Path to directory containing label images.\n"
                "Mandatory --outdir: Output path.\n"
                "Optional -c: Count of neighboring pixels, required to merge labels."
                "\n"
                "Example: ./merge_labels --dir /path/to/image_folder/ --outdir /path/to/out/folder/ -c 10"
                "\n";
        return 1;
    }

    string directory = Parser::getOption("--dir");
    string out_directory = Parser::getOption("--outdir");
    int neighborCnt = 10;
    if(Parser::hasOption("-c")) neighborCnt = Parser::getIntOption("-c");

    for(int currentFrame=0; ; currentFrame++){

        stringstream ss;
        ss << setw(5) << setfill('0') << currentFrame;
        std::string indexStr = ss.str();

        string impath = directory + "/" + indexStr + ".png";
        string outpath = out_directory + "/" + indexStr + ".png";
        if(!exists(impath)) break;
        Mat input = imread(impath);

        // Count neighboring colors
        vector<Vec3b> colors;
        findColors(input, colors);
        Mat neighborCounts = countColorNeighbors(input, colors);
        vector<int> replaceList(colors.size());
        //cout << neighborCounts << endl;

        // Build replace hierarchy
        for (size_t i = 0; i < colors.size(); ++i) replaceList[i] = i;
        for (int i = 0; i < neighborCounts.rows; ++i){
            for (int j = i+1; j < neighborCounts.cols; ++j){
                if(neighborCounts.at<int>(i,j) >= neighborCnt){
                    int index = i;
                    do{
                        int nextNode = replaceList[index];
                        if(replaceList[index] < j) replaceList[index] = j;
                        if(nextNode==index) break;
                        index = nextNode;
                    }while(true);
                    //if(replaceList[i]!=i) bug
                    //replaceList[i] = j;
                }
            }
        }

        // Build color mapping
        vector<Vec3b> colorsOld;
        vector<Vec3b> colorsNew;
        auto findReplaceIndex = [&replaceList](int index) -> int {
            while(true) {
                if(index == replaceList[index]) return index;
                index = replaceList[index];
            }
        };
        for (size_t i = 0; i < colors.size(); ++i) {
            int r = findReplaceIndex(i);
            if(r!=(int)i){
                colorsOld.push_back(colors[i]);
                colorsNew.push_back(colors[r]);
            }
        }

        // Execute color mapping
        for (size_t i = 0; i < input.total(); ++i) {
            Vec3b& pixel = input.at<Vec3b>(i);
            int r = getColorIndex(colorsOld, pixel);
            if(r>=0) pixel = colorsNew[r];
        }

        imwrite(outpath, input);
    }

    return 0;
}
