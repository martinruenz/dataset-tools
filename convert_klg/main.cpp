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

/**
 * KLG-files start with a:
 * int32_t: frame count
 *
 * Afterwards, for each frame:
 * int64_t: timestamp (typically in microseconds)
 * int32_t: depthSize
 * int32_t: imageSize
 * depthSize * unsigned char: depth_compress_buf (typically 16bit per pixel, typically in mm)
 * imageSize * unsigned char: encodedImage->data.ptr
 *
 * See: https://github.com/mp3guy/Logger2/blob/master/src/Logger2.h
 */

#include "../common/common.h"
#include "../common/common_3d.h"
#include "JPEGLoader.h"
#include <zlib.h>

using namespace std;
using namespace cv;

int main(int argc, char * argv[])
{
    //return 0; // outsourced computations, test if it is still working.
    Parser::init(argc, argv);
    //printf("Running with OpenCV: %s", cv::getBuildInformation().c_str());

    if(!Parser::hasOption("-i") || !Parser::hasOption("-o")){
        cout << "Error, invalid arguments.\n"
                "Mandatory -i: Input klg file.\n"
                "Mandatory -o: Output directory.\n"
                "Optional -a: Extract image files (jpg/png+exr).\n"
                "Optional -n: Only extract image files (jpg/png+exr).\n"
                "Optional -m: Min depth in mm (default value: 2).\n"
                "Optional -s: Silent. Don't show frames during export.\n"
                "Optional -w: Image width (default value: 640).\n"
                "Optional -h: Image height (default value: 480).\n"
                "Optional -cx: Optical center x (default value: 320).\n"
                "Optional -cy: Optical center y (default value: 240).\n"
                "Optional -fx: Focal length x (default value: 528).\n"
                "Optional -fy: Focal length y (default value: 528).\n"
                "Optional -f: Numer of frame (only this frame is processed).\n"
                "Optional -png: Export PNG instead of JPG.\n"
                "Optional -tum: Export in TUM format, when exporting frames.\n"
                "Optional -depthpng: Export depth images as 16-bit greyscale PNG instead of float exr.\n"
                "Optional -depthmin: Min depth value for export.\n"
                "Optional -depthscale: Multiply depth values with this factor, before writing PNGs. [1=mm,0.1=m...] (Default: 5, as in TUM).\n"
                "5000"
                "\n"
                "Example: ./convert_klgToPly -i test.klg -o /path/to/output_folder/" << endl;
        return 1;
    }

    string outputDir = Parser::getOption("-o");
    string inputFile = Parser::getOption("-i");

    if(!exists(inputFile) || !exists(outputDir)) {
        cout << "Invalid parameters." << endl;
        return 1;
    }

    PinholeParameters intrinsics;
    intrinsics.cx = 320;
    intrinsics.cy = 240;
    intrinsics.fx = 528;
    intrinsics.fy = 528;
    float depthmin = Parser::getFloatOption("-depthmin",  2.0 / 1000);
    float depthscale = Parser::getFloatOption("-depthscale", 5);
    unsigned width = 640;
    unsigned height = 480;
    const int numPixel = width*height;
    bool alsoImages = Parser::hasOption("-a");
    bool noPointCloud = Parser::hasOption("-n");
    bool silent = Parser::hasOption("-s");
    bool depthPNG = Parser::hasOption("-depthpng");
    bool tumFormat = Parser::hasOption("-tum");
    const string depthFileExt = depthPNG ? ".png" : ".exr";
    const string colorFileExt = Parser::hasOption("-png") ? ".png" : ".jpg";
    if(Parser::hasOption("-m")) depthmin = 0.001 * Parser::getFloatOption("-m");
    if(Parser::hasOption("-w")) width = Parser::getIntOption("-w");
    if(Parser::hasOption("-h")) height = Parser::getIntOption("-h");
    if(Parser::hasOption("-cx")) intrinsics.cx = Parser::getFloatOption("-cx");
    if(Parser::hasOption("-cy")) intrinsics.cy = Parser::getFloatOption("-cy");
    if(Parser::hasOption("-fy")) intrinsics.fy = Parser::getFloatOption("-fy");
    if(Parser::hasOption("-fx")) intrinsics.fx = Parser::getFloatOption("-fx");

    if(noPointCloud && !alsoImages) {
        cout << "Invalid parameters: -n requires -a" << endl;
        return 0;
    }

    int numFrames = 0;
    unsigned char* depthReadBuffer;
    unsigned char* imageReadBuffer;
    unsigned char* decompressionBufferDepth;
    unsigned char* decompressionBufferImage;
    int32_t depthSize = 0;
    int32_t imageSize = 0;
    int64_t timestamp;
    JPEGLoader jpeg;

    std::ofstream fDList, fRGBList, fAssociations;
    if(tumFormat){
        fDList.open(outputDir+"/depth.txt");
        fRGBList.open(outputDir+"/rgb.txt");
        fAssociations.open(outputDir+"/associations.txt");
        boost::filesystem::create_directories(outputDir+"/rgb");
        boost::filesystem::create_directories(outputDir+"/depth");
    }
    FILE* fp = fopen(inputFile.c_str(), "rb");
    CHECK_THROW(fread(&numFrames, sizeof(int32_t), 1, fp));

    depthReadBuffer = new unsigned char[numPixel * 2];
    imageReadBuffer = new unsigned char[numPixel * 3];
    decompressionBufferDepth = new unsigned char[numPixel * 2];
    decompressionBufferImage = new unsigned char[numPixel * 3];

    cout << "Start working on KLG file with " << numFrames << " frames..." << endl;

    float progressStep = 1.0 / (numFrames+1);
    size_t numErrors = 0;
    int currentFrame=0;
    if(Parser::hasOption("-f")){
        int frame = Parser::getIntOption("-f");
        while(currentFrame < frame && currentFrame < numFrames) {
            CHECK_THROW(fread(&timestamp, sizeof(int64_t), 1, fp));
            CHECK_THROW(fread(&depthSize, sizeof(int32_t), 1, fp));
            CHECK_THROW(fread(&imageSize, sizeof(int32_t), 1, fp));
            CHECK_THROW(fread(depthReadBuffer, depthSize, 1, fp));
            if(imageSize > 0) CHECK_THROW(fread(imageReadBuffer, imageSize, 1, fp));

            currentFrame++;
            showProgress(currentFrame*progressStep);
        }
        numFrames = currentFrame+1;
    }

    for(; currentFrame < numFrames; currentFrame++){

        //cout << "Start processing frame " << currentFrame << endl;
        stringstream ss;
        ss << setw(4) << setfill('0') << currentFrame;
        std::string indexStr = ss.str();

        // Extract
        CHECK_THROW(fread(&timestamp, sizeof(int64_t), 1, fp));
        CHECK_THROW(fread(&depthSize, sizeof(int32_t), 1, fp));
        CHECK_THROW(fread(&imageSize, sizeof(int32_t), 1, fp));
        CHECK_THROW(fread(depthReadBuffer, depthSize, 1, fp));
        if(imageSize > 0) CHECK_THROW(fread(imageReadBuffer, imageSize, 1, fp));

        if(depthSize == numPixel * 2){
            memcpy(&decompressionBufferDepth[0], depthReadBuffer, numPixel * 2);
        }else{
            unsigned long decompLength = numPixel * 2;
            uncompress(&decompressionBufferDepth[0], (unsigned long *)&decompLength, (const Bytef *)depthReadBuffer, depthSize);
        }

        if(imageSize == numPixel * 3) memcpy(&decompressionBufferImage[0], imageReadBuffer, numPixel * 3);
        else if(imageSize > 0) jpeg.readData(imageReadBuffer, imageSize, (unsigned char *)&decompressionBufferImage[0]);
        else throw std::invalid_argument("Invalid data.");

        Mat depth(height, width, CV_16UC1, (unsigned short*)&decompressionBufferDepth[0]);
        Mat rgb(height, width, CV_8UC3, (unsigned char*)&decompressionBufferImage[0]);
        cvtColor(rgb, rgb, CV_BGR2RGB);

        if(!silent){
            cv::imshow("Depth", depth);
            cv::waitKey(1);
            cv::imshow("RGB", rgb);
            cv::waitKey(1);
        }

        cv::Mat depthMetric;
        depth.convertTo(depthMetric, CV_32FC1, 0.001);

        // Optional image export
        if(alsoImages){
            std::string depthPath, rgbPath, ts;
            if(tumFormat){
                ts = to_string(timestamp);//timestamp / double(1e6);
                ts.insert(ts.end()-6,'.');
                depthPath = "depth/"+ts+depthFileExt;
                rgbPath = "rgb/"+ts+colorFileExt;
                fDList << ts << " " << depthPath << endl;
                fRGBList << ts << " " << rgbPath << endl;
                fAssociations << ts << " " << rgbPath << " " << ts << " " << depthPath << endl;
                depthPath = outputDir+"/"+depthPath;
                rgbPath = outputDir+"/"+rgbPath;
            } else {
                depthPath = outputDir+"/Depth"+indexStr+depthFileExt;
                rgbPath = outputDir+"/Color"+indexStr+colorFileExt;
            }
            cv::imwrite(rgbPath, rgb);
            if(depthPNG) imwrite(depthPath, depthscale * depth);
            else imwrite(depthPath, depthMetric); //storeFloatImage(depthMetric, depthPath, 0, 50);
        }

        // 3D generation
        if(!noPointCloud){
            Projected3DCloud points(depthMetric, rgb, intrinsics, depthmin);
            points.toPly(outputDir+"/Points"+indexStr+".ply");
        }
        showProgress(currentFrame*progressStep);
    }

    cout << "\nDone. Errors: " << numErrors << endl;

    delete[] depthReadBuffer;
    delete[] imageReadBuffer;
    delete[] decompressionBufferDepth;
    delete[] decompressionBufferImage;
    fclose(fp);
    if(tumFormat){
        fDList.close();
        fRGBList.close();
        fAssociations.close();
    }

    return 0;
}
