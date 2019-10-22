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

/** Input:
    mask-images (labels1, labels2)
    label-selection (pLabel1, pLabel2), optional

    Output:
    Mapping from label -> (intersection, union) (results)
*/
void intersectionOverUnion(Mat labels1,
                           Mat labels2,
                           map<unsigned char, pair<unsigned,unsigned>>& results,
                           uchar* pLabel1 = nullptr,
                           uchar* pLabel2 = nullptr){
    // Checks
    if(labels1.cols != labels2.cols) throw invalid_argument("Images do not match.");
    if(labels1.rows != labels2.rows) throw invalid_argument("Images do not match.");
    if(labels1.type() != labels2.type()) throw invalid_argument("Images do not match.");
    if(labels1.type() != CV_8UC1) {
        if(labels1.type() == CV_8UC3){
            static bool warned = false;
            if(!warned) cout << "\033[0;31mWARNING: Converting 3 channels to 1. Make sure this behaviour is as you expect!\033[0m" << std::endl;
            warned = true;
            // Make greyscale
            //cvtColor(labels1,labels1, CV_RGB2GRAY);
            //cvtColor(labels2,labels2, CV_RGB2GRAY);

            // Use single channel
            cv::extractChannel(labels1, labels1, 0);
            cv::extractChannel(labels2, labels2, 0);
        } else {
            throw invalid_argument("Images invalid. (Type: " + cvTypeToString(labels1.type()) + ")");
        }
    }
    if((pLabel1 && !pLabel2) || (!pLabel1 && pLabel2)) throw invalid_argument("Only 1 label provided.");

    vector<unsigned> unionCounts(256,0);
    vector<unsigned> intersectionCounts(256,0);
    bool compareSpecifiy = pLabel1 && pLabel2;

    // Compare all labels
    for(size_t i = 0; i < labels1.total(); i++){
        unsigned char v1 = labels1.data[i];
        unsigned char v2 = labels2.data[i];
        if(compareSpecifiy){
            v1 = (*pLabel1 == v1);
            v2 = (*pLabel2 == v2);
        }
        if(v1==v2){
            unionCounts[v1]++;
            intersectionCounts[v1]++;
        } else {
            unionCounts[v1]++;
            unionCounts[v2]++;
        }
    }

    // Write result
    for (int i = 0; i < 256; ++i)
        if(unionCounts[i] > 0) results[i] = pair<unsigned,unsigned>(intersectionCounts[i], unionCounts[i]);
}

int main(int argc, char * argv[])
{
    Parser parser(argc, argv);
    if(!parser.hasOption("--dir") || !parser.hasOption("--dirgt")){
        cout << "Error, invalid arguments.\n"
                "Mandatory --dir: Your segmentation results.\n"
                "Mandatory --dirgt: Ground-truth segmentation.\n"
                "Optional --prefix: Prefix of your images\n"
                "Optional --prefixgt: Prefix of ground-truth images\n"
                "Optional --starti: start-index\n"
                "Optional --width: your index-width\n"
                "Optional --widthgt: ground-truth index-width\n"
                "Optional --outtxt: create a text-file with per image data.\n"
                "Optional --label: this label will be treated as foreground (=1), everything else is background. \n"
                "Optional --labelgt: this ground-truth label will be treated as foreground (=1), everything else is background.\n"
                "Optional -v: Be verbose.\n"
                "\n"
                "This tool tries to match all labels in both images, except if you provide --labelgt and --label."
                "In the latter case only the specified labels, possibly having unmatching IDs, are compared.";
        return 0;
    }

    bool verbose = parser.hasOption("-v");

    string directory = parser.getDirOption("--dir");
    string gt_directory = parser.getDirOption("--dirgt");

    string prefix = parser.getOption("--prefix");
    string gt_prefix = parser.getOption("--prefixgt");

    int index_width = parser.getIntOption("--width");
    int gt_index_width = parser.getIntOption("--widthgt");
    int start_index = parser.getIntOption("--starti");

    uchar gt_label = parser.getUCharOption("--labelgt");
    uchar label = parser.getUCharOption("--label");
    uchar* pgt_label = parser.hasOption("--labelgt") ? &gt_label : nullptr;
    uchar* plabel = parser.hasOption("--label") ? &label : nullptr;

    vector<pair<unsigned,unsigned>> totals(256,{0,0});
    vector<float> avg(256,0);
    vector<unsigned> seenCnt(256,0);

    ofstream file;
    if(parser.hasOption("--outtxt")) file.open(parser.getOption("--outtxt"), ofstream::out | ofstream::app);

    for(int i = start_index; ; i++){
        stringstream ss_gtbase, ss_base;
        ss_base << directory << prefix << setw(index_width) << setfill('0') << i << ".png";
        ss_gtbase << gt_directory << gt_prefix << setw(gt_index_width) << setfill('0') << i << ".png";
        cv::Mat gt = imread(ss_gtbase.str());
        cv::Mat seg = imread(ss_base.str());

        if(!gt.total() || !seg.total()) {
            if(verbose) cout << "Could not find on of the following path:\n" << ss_gtbase.str() << "\n" << ss_base.str() << "\nDone.";
            break;
        }

        if(verbose) cout << "Evaluation image " << i << "..." << std::endl;
        map<unsigned char, pair<unsigned,unsigned>> res;
        intersectionOverUnion(seg,gt,res,plabel,pgt_label);

        int lastl = 0;
        file << i << "\t";
        for(auto& r : res){
            float iou = r.second.first / float(r.second.second);
            if(verbose) cout << "\tLabel " << (int)r.first << ":\t" << iou << "\n";
            avg[r.first] += iou;
            seenCnt[r.first]++;
            totals[r.first].first += r.second.first;
            totals[r.first].second += r.second.second;
            for (int j = lastl; j < r.first; ++j) file << "\t";
            file << iou;
            lastl = r.first;
        }
        file << "\n";
        if(verbose) cout << endl;
    }

    cout << "Overall result: \n";
    for (int i = 0; i < 256; ++i) {
        if(totals[i].second > 0) {
            cout << "\tLabel " << (int)i << ":\t" << totals[i].first / float(totals[i].second) << "\t\t(avg. " << avg[i] / float(seenCnt[i]) << ")\n";
        }
    }
    file.close();

    return 0;
}
