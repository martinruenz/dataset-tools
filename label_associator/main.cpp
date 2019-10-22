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

struct LabelDescription {
    cv::Point2f center;
    unsigned label;
    unsigned class_id; //TODO implement
};

//unsigned getLabelDescrIndex(const vector<LabelDescription>& descriptions, const Vec3b& color){
//    for(size_t i = 0; i < descriptions.size(); i++)
//        if(color==descriptions[i].color)
//            return i;
//    return -1;
//}

// From new description indexes to old ones, new label-indexes are stored separately
pair<map<int, int>, std::vector<int>> mapLabels(const std::vector<LabelDescription>& prevLabels, const vector<LabelDescription>& newLabels, float maxDist){
    map<int, int> result;
    vector<int> newIndexes;
    vector<bool> available(prevLabels.size(), true);
    for (size_t j = 0; j < newLabels.size(); ++j) {
        int bestIndex = -1;
        float bestDistance = numeric_limits<float>::max();
        for (size_t i = 0; i < prevLabels.size(); ++i) {
            if(available[i]){
                float dist = norm(prevLabels[i].center - newLabels[j].center);
                if(dist <= maxDist && bestDistance > dist){
                    bestIndex = i;
                    bestDistance = dist;
                }
            }
        }
        if(bestIndex >= 0){
            available[bestIndex] = false;
            result[j] = bestIndex;
        } else {
            newIndexes.push_back(j);
        }
    }
    return {result, newIndexes};
}


int main(int argc, char * argv[])
{
    Parser parser(argc, argv);

    if(!parser.hasOption("--dir") || !parser.hasOption("--outdir")){
        cout << "Error, invalid arguments.\n"
                "Mandatory --dir: Path to directory containing label images.\n"
                "Mandatory --outdir: Output path.\n"
                "Optional --maxDist: float which describes the max differences of centers of labels.\n"
                "NOT IMPLEMENTED: Optional --classdir: Path to directory containing class-ids. Labels are only associated, iff they are of the same class.\n"
                "\n"
                "Example: ./associate_labels --dir /path/to/image_folder/ --classdir /path/to/classdir/ --outdir /path/to/out/folder/ -c 10"
                "\n";
        return 1;
    }

    string directory = parser.getOption("--dir");
    string out_directory = parser.getOption("--outdir");
    string class_directory = parser.getOption("--classdir");
    float maxDist = parser.getFloatOption("--maxDist", numeric_limits<float>::max());

    std::vector<LabelDescription> prevLabels;
    unsigned nextNewLabelID = 1;

    for(int currentFrame=0; ; currentFrame++){

        stringstream ss;
        ss << setw(5) << setfill('0') << currentFrame;
        std::string indexStr = ss.str();

        string impath = directory + "/" + indexStr + ".png";
        string outpath = out_directory + "/" + indexStr + ".png";
        if(!exists(impath)) {
            cout << "Reached end. (file: " << impath << " does not exist)" << endl;
            break;
        }
        if(exists(outpath)) {
            cerr << "Output file " << outpath << " already exists." << endl;
            break;
        }

        Mat input_color = imread(impath);
        vector<cv::Vec3b> colorTable;
        Mat input_labels = colourToLabelImage(input_color, colorTable);

        std::vector<LabelDescription> newLabels;
        std::vector<ComponentData> ccStats;
        Mat components = connectedLabels(input_labels, &ccStats);
        std::map<int, std::list<int>> labelToComponents = mapLabelsToComponents(ccStats);

        auto eraseComponent = [&](int compIndex){
            const ComponentData& comp = ccStats[compIndex];
            for(int y = comp.top; y <= comp.bottom; y++){
                for(int x = comp.left; x <= comp.right; x++){
                    if(compIndex == components.at<int>(y,x)){
                        input_labels.at<uchar>(y,x) = 0;
                    }
                }
            }
        };

        // Only keep largest component of each label
        for(auto& lc : labelToComponents) {
            if(lc.second.size() > 1) {
                int keepIndex = -1;
                int keepSize = 0;
                // Find biggest
                for(int index : lc.second){
                    if(ccStats[index].size > keepSize){
                        keepSize = ccStats[index].size;
                        keepIndex = index;
                    }
                }
                // Erase others
                for(int index : lc.second)
                    if(index != keepIndex) {
                        eraseComponent(index);
                        ccStats[index].size = 0;
                    }

                //auto it = lc.second.begin();
                //while (it != lc.second.end()){
                //    if(*it != keepIndex){
                //        eraseComponent(*it);
                //        ccStats[*it].size = 0;
                //        it = lc.second.erase(it);
                //    } else {
                //        ++it;
                //    }
                //}
            }
        }

        for(const ComponentData& comp : ccStats)
            if(comp.size > 0 && comp.label != 0)
                newLabels.push_back({ Point2f(comp.centerX, comp.centerY), comp.label });

        pair<map<int, int>, std::vector<int>> mapping = mapLabels(prevLabels, newLabels, maxDist);

        map<int, int> appliedMap;

        // Handle label mapping
        for(auto& m : mapping.first){
            appliedMap[newLabels[m.first].label] = prevLabels[m.second].label;
            newLabels[m.first].label = prevLabels[m.second].label;
        }

        // Handle new labels
        for(auto& n : mapping.second){
            if(nextNewLabelID > 255) //throw invalid_argument("Too many labels. Only up to 255 are supported.");
                cout << "Warning only tested with < 256 labels." << endl;
            appliedMap[newLabels[n].label] = nextNewLabelID;
            newLabels[n].label = nextNewLabelID;
            nextNewLabelID++;
        }

        Mat out(input_color.rows, input_color.cols, CV_16UC1);

        for (size_t i = 0; i < out.total(); ++i) {
            if(input_labels.at<uchar>(i) == 0) out.at<unsigned short>(i) = 0;
            else out.at<unsigned short>(i) = appliedMap[input_labels.at<uchar>(i)];
        }

        imwrite(outpath, labelToColourImage(out));

        imshow("Output", labelToColourImage(out));
        imshow("Input", input_color);
        waitKey(1);

        prevLabels = newLabels;
    }

    cout << "Found " << nextNewLabelID-1 << " unique labels after association." << endl;


    return 0;
}
