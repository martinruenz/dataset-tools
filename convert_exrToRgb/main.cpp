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

bool convertFile(const std::string& input, const std::string& output, float min, float max){
  if(exists(output)) {
      cerr << "File " << output << " already exists." << std::endl;
      return false;
    }

  Mat raw = imread(input, cv::IMREAD_UNCHANGED);
  Mat converted;

  if(raw.channels() == 3){
      vector<Mat> channels(3);
      split(raw, channels);
      for (int i = 0; i < 3; ++i) channels[i] = floatToUC1(channels[i], min, max);
      //std::swap(channels[0],channels[2]);
      merge(channels, converted);
    } else {
      converted = floatToUC1(raw, min, max);
    }

  imwrite(output, converted);

  return true;
}

int main(int argc, char * argv[])
{
  Parser::init(argc, argv);

  if(!Parser::hasOption("-i") || !Parser::hasOption("-o")) {
    cout << "Error, invalid arguments.\n"
            "Mandatory -i: Input image or directory.\n"
            "Mandatory -o: Output image or directory.\n"
            "Optional -min: Input is mapped from 'min-max' to '0-255', min default: 0.0\n"
            "Optional -max: Input is mapped from 'min-max' to '0-255', max default: 1.0\n"
            "\n"
            "Example: ./convert_exrToRgb -i /path/to/input.exr -o /path/to/output.jpg";

    return 1;
  }

  string input = Parser::getOption("-i");
  string output = Parser::getOption("-o");
  float min = Parser::hasOption("-min") ? Parser::getFloatOption("-min") : 0;
  float max = Parser::hasOption("-max") ? Parser::getFloatOption("-max") : 1;

  if(exists(input) && is_directory(input) ) {
    if(!exists(output) || !is_directory(output))
      throw std::invalid_argument("Input / ouput should both be files, or both be directories.");
    std::vector<string> files = getFilenames(input, { ".exr" });
    float progressStep = 1.0 / (files.size()+1);
    unsigned i = 0;
    for(string& file : files){
      convertFile(input + "/" + file, output + "/" + file + ".png", min, max);
      showProgress(progressStep*i++);
    }
  } else {
    convertFile(input, output, min, max);
  }

  return 0;
}
