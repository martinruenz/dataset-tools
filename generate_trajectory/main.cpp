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
#include "../common/common_random.h"
#include <fstream>

using namespace Eigen;
using namespace std;

struct Trajectory {

    Trajectory(size_t size_hint){
        positions.reserve(size_hint);
        orientations.reserve(size_hint);
    }
    vector<Vector3f> positions;
    vector<Vector3f> orientations;
    void addGaussianNoise(float std_translation, float std_rotation, float fraction=1.0f) {
        GaussianNoise noise_t(0, std_translation);
        GaussianNoise noise_r(0, std_rotation);
        for(size_t i=0; i<positions.size(); i++){
            if(randomCoin(fraction)){
                positions[i] += Vector3f(noise_t.generate(), noise_t.generate(), noise_t.generate());
                orientations[i] += Vector3f(noise_r.generate(), noise_r.generate(), noise_r.generate());
            }
        }
    }

    inline friend ostream& operator<<(ostream& os, const Trajectory& trajectory){
        assert(trajectory.positions.size() == trajectory.orientations.size());
        for(unsigned i=0; i<trajectory.positions.size(); i++){
            os << trajectory.positions[i](0) << " " << trajectory.positions[i](1) << " " << trajectory.positions[i](2) << " "
               << trajectory.orientations[i](0) << " " << trajectory.orientations[i](1) << " " << trajectory.orientations[i](2);
            os << "\n";
        }
        return os;
    }
};

void save_trajectory(const string& name, const Trajectory& trajectory, const string& postfix = ""){
    if(name.size()){
        string file = name + postfix + ".txt";
        if(exists(file)){
            cerr << "Not saving " << file << ", file already exists." << endl;
        } else {
            ofstream os(file);
            os << trajectory;
            os.close();
        }
    }
}

int main(int argc, char * argv[])
{
    srand(time(NULL));
    Parser parser(argc, argv);

    if(!parser.hasOption("-n")) {
        cout << "Error, invalid arguments.\n"
                "Mandatory -n: Number of poses.\n"
                "Optional --std_t: Add noise with this standard deviation to positions.\n"
                "Optional --std_r: Add noise with this standard deviation to orientations.\n"
                "Optional --file: Create <file>.txt and potentially <file>_noisy.txt in current dir.\n"
                "\n"
                "Example: ./generate_trajectory";

        return 1;
    }

    int n = parser.getIntOption("-n");
    float std_t = parser.getFloatOption("--std_t");
    float std_r = parser.getFloatOption("--std_r");
    float outlier_fraction = parser.getFloatOption("--outlier_fraction", 0.0f);
    float max_angular_velocity = parser.getFloatOption("--max_angular_velocity", 0.03f);
    float max_angular_acceleration = parser.getFloatOption("--max_angular_acceleration", max_angular_velocity / 30.0f);
    float max_velocity = parser.getFloatOption("--max_velocity", 0.05f);
    float max_acceleration = parser.getFloatOption("--max_acceleration", max_velocity / 30.0f);
    float scale_initial_position = parser.getFloatOption("--scale_initial_position", 0);
    float scale_initial_rotation = parser.getFloatOption("--scale_initial_rotation", 0);
    float scale_initial_speed = parser.getFloatOption("--scale_initial_speed", 0);
    float scale_initial_angular_speed = parser.getFloatOption("--scale_initial_angular_speed", 0.3f);
    float scale_initial_acceleration = parser.getFloatOption("--scale_initial_angular_acceleration", 0.1f * max_acceleration);
    float scale_initial_angular_acceleration = parser.getFloatOption("--scale_initial_acceleration", 0.1f * max_angular_acceleration);
    string file = parser.getOption("--file");
    if(n <= 0) throw std::invalid_argument("Number of poses has to be > 0.");

    // initial speed & acceleration
    Trajectory trajectory(n);
    Vector3f angular_speed = scale_initial_angular_speed * Vector3f::Random();
    Vector3f speed = scale_initial_speed * Vector3f::Random();
    Vector3f angular_acceleration = scale_initial_acceleration * Vector3f::Random();
    Vector3f acceleration = scale_initial_angular_acceleration * Vector3f::Random();
    Vector3f position = scale_initial_position * Vector3f::Random();
    Vector3f rotation = scale_initial_rotation * Vector3f::Random();

    for(int i=0; i<n; i++){
        angular_speed += angular_acceleration;
        speed += acceleration;
        acceleration += 0.1f * max_acceleration * Vector3f::Random();
        angular_acceleration += 0.1f * max_angular_acceleration * Vector3f::Random();
        if(acceleration.norm() > max_acceleration) acceleration = max_acceleration * acceleration.normalized();
        if(angular_acceleration.norm() > max_angular_acceleration) angular_acceleration = max_angular_acceleration * angular_acceleration.normalized();
        position += speed;
        rotation += angular_speed;
        if(speed.norm() > max_velocity) speed = max_velocity * speed.normalized();
        if(angular_speed.norm() > max_angular_velocity) angular_speed = max_angular_velocity * angular_speed.normalized();
        trajectory.positions.push_back(position);
        trajectory.orientations.push_back(rotation);
    }

    cout << "Generated trajectory:\n";
    cout << trajectory;
    save_trajectory(file, trajectory);

    if(std_r > 0 || std_t > 0){
        trajectory.addGaussianNoise(std_t, std_r);
        if(outlier_fraction > 0.0f)
            trajectory.addGaussianNoise(10 * std_t, 10 * std_r, outlier_fraction);
        cout << "Noisy trajectory:\n";
        cout << trajectory;
        save_trajectory(file, trajectory, "_noisy");
    }
    return 0;
}
