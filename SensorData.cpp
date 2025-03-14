//
// Created by Hamza El-Kebir on 2/7/25.
//

#include "SensorData.hpp"

void SensorData::normalizeNormal(SensorData &data) {
    float n_norm = sqrt(data.nx*data.nx + data.ny*data.ny + data.nz*data.nz);
    data.nx /= n_norm;
    data.ny /= n_norm;
    data.nz /= n_norm;
}

void SensorData::convertFromEuler(SensorData &data) {
    float psi = data.nx * DEG_TO_RAD; // azimuth (Z-rot)
    float theta = data.ny * DEG_TO_RAD; // elevation (Y'-rot)
    float phi = data.nz * DEG_TO_RAD; // roll (X''-rot)

    float ca = cos(psi);
    float ce = cos(theta);
    float cr = cos(phi);

    float sa = sin(psi);
    float se = sin(theta);
    float sr = sin(phi);

    // Direction cosine matrix based on Z-Y-X rotation sequence
    /*[[ca*se, ca*se*sr - sa*cr, ca*se*cr + sa*sr],
       [sa*ce, ca*cr + sa*se*sr, sa*se*cr - ca*sr],
       [-se  , ce*sr           , ce*cr]
      ] */

    // Compute unit vector
    data.nx = -(ca*se*cr + sa*sr);
    data.ny = -(sa*se*cr - ca*sr);
    data.nz = -(ce*cr);
}

SensorData SensorDataExtended::convert() const {
    SensorData data;

    data.sensor_id = sensor_id;
    data.time = time;

    data.x = x;
    data.y = y;
    data.z = z;

    data.nx = nx;
    data.ny = ny;
    data.nz = nz;

    return data;
}

SensorData SensorDataExtended::convertV() const {
    SensorData data;

    data.sensor_id = sensor_id;
    data.time = time;

    data.x = vx;
    data.y = vy;
    data.z = vz;

    data.nx = nx;
    data.ny = ny;
    data.nz = nz;

    return data;
}

void SensorDataExtended::normalizeNormal(SensorDataExtended &data) {
    float norm = std::sqrt(data.nx * data.nx + data.ny * data.ny + data.nz * data.nz);
    if (norm > 0) {
        data.nx /= norm;
        data.ny /= norm;
        data.nz /= norm;
    }
}
