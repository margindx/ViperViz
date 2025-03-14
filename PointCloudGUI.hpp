//
// Created by Hamza El-Kebir on 2/11/25.
//

#ifndef VIPER_POINTCLOUDGUI_HPP
#define VIPER_POINTCLOUDGUI_HPP

#include "include/raylib-cpp.hpp"
#include "ZMQServer.hpp"
#include "SensorData.hpp"

class PointCloudGUI {
public:
    static Color sensorColors[8];

    static bool recordingPoints;
    static bool doBuffer;
    static bool doDrawLine;

    static int launch();

    static int launchRealTime();

    static void drawSensorData(SensorData &data, double r=0.1, double l=0.1);
};


#endif //VIPER_POINTCLOUDGUI_HPP
