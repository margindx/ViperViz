//
// Created by Hamza El-Kebir on 2/7/25.
//

#ifndef VIPER_SENSORDATA_HPP
#define VIPER_SENSORDATA_HPP

#include <cstddef>
#include <cmath>

#define DEG_TO_RAD (float) (M_PI / 180.0)

// Define the SensorData struct
struct SensorData {
    int sensor_id;
    long long time;
    float x, y, z;
    float nx, ny, nz;
    float qw, qx, qy, qz;

    static void convertFromEuler(SensorData &data);

    static void normalizeNormal(SensorData &data);
};

struct SensorDataExtended {
    int sensor_id;
    long long time;
    float x, y, z;
    float nx, ny, nz;
    float vx, vy, vz;  // Velocity for position
    float vnx, vny, vnz; // Velocity for normal vector

    SensorData convert() const;

    // Convert velocity data
    SensorData convertV() const;

    static void normalizeNormal(SensorDataExtended &data);
};


#endif //VIPER_SENSORDATA_HPP
