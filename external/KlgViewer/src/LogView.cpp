#include "Resolution.h"
#include "RawLogReader.h"
#include <fstream>

int find_argument(int argc, char** argv, const char* argument_name)
{
    for(int i = 1; i < argc; ++i)
    {
        if(strcmp(argv[i], argument_name) == 0)
        {
            return i;
        }
    }
    return -1;
}

int parse_argument (int argc, char** argv, const char* str, int &val)
{
    int index = find_argument (argc, argv, str) + 1;

    if (index > 0 && index < argc )
      val = atoi (argv[index]);

    return (index - 1);
}

int parse_argument (int argc, char** argv, const char* str, std::string &val)
{
    int index = find_argument (argc, argv, str) + 1;

    if (index > 0 && index < argc )
      val = argv[index];

    return index - 1;
}

int main(int argc, char * argv[])
{
    int width = 640;
    int height = 480;
    int pauseMS = 33;
    bool switchColor = false;

    parse_argument(argc, argv, "-w", width);
    parse_argument(argc, argv, "-h", height);
    parse_argument(argc, argv, "-p", pauseMS);
    switchColor = find_argument(argc, argv, "-s") > -1;


    Resolution::getInstance(width, height);

    Bytef * decompressionBuffer = new Bytef[Resolution::getInstance().numPixels() * 2];
    IplImage * deCompImage = 0;

    std::string logFile;
    assert(parse_argument(argc, argv, "-l", logFile) > 0 && "Please provide a log file");

    RawLogReader logReader(decompressionBuffer,
                           deCompImage,
                           logFile,
                           find_argument(argc, argv, "-f") != -1);

    cv::Mat1b tmp(height, width);
    cv::Mat3b depthImg(height, width);

    unsigned currentFrame = 1;
    while(logReader.hasMore())
    {
        usleep(pauseMS * 1000);
        std::cout << "Showing frame: " << currentFrame++ << "(ts: " << logReader.timestamp << ")" << std::endl;
        logReader.getNext();

        cv::Mat3b rgbImg(height, width, (cv::Vec<unsigned char, 3> *)logReader.deCompImage->imageData);
        if(switchColor) cvtColor(rgbImg, rgbImg, CV_BGR2RGB);

        cv::Mat1w depth(height, width, (unsigned short *)&decompressionBuffer[0]);

        cv::normalize(depth, tmp, 0, 255, cv::NORM_MINMAX, 0);

        cv::cvtColor(tmp, depthImg, CV_GRAY2RGB);

        cv::imshow("RGB", rgbImg);

        cv::imshow("Depth", depthImg);

        char key = cv::waitKey(1);

        if(key == 'q')
            break;
        else if(key == ' ')
            key = cv::waitKey(0);
            if(key == 'q')
                break;
    }

    delete [] decompressionBuffer;

    if(deCompImage)
    {
        cvReleaseImage(&deCompImage);
    }

    return 0;
}
