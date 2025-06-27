#include <iostream>
#include "viper_ui.h"
#include "ZMQServer.hpp"
#include "PointCloudGUI.hpp"

int main() {
//    std::cout << "HEMISPHERE_CONFIG size " << sizeof(HEMISPHERE_CONFIG) << std::endl;
//    viper_ui ui;
//    ui.detect_input();

   ZMQServer::init(5555, ZMQServer::Publisher);
   ZMQServer::serve();
x
//    ZMQServer::init(5555, ZMQServer::Subscriber);
//    ZMQServer::subscribeToSensor(1);
//    ZMQServer::subscribeToSensor(2);
//    ZMQServer::subscribeToSensor(3);
//    ZMQServer::subscribeToSensor(4);
//    ZMQServer::consume();

    // ZMQServer::verbose = false;
    // ZMQServer::init(5555, ZMQServer::Subscriber);
    // ZMQServer::subscribeToSensor(1);
    // ZMQServer::subscribeToSensor(2);
    // ZMQServer::subscribeToSensor(3);
    // ZMQServer::subscribeToSensor(4);
    // ZMQServer::log();

//    ZMQServer::init(5555, ZMQServer::Subscriber);
//    ZMQServer::subscribeToSensor(1);
//    ZMQServer::subscribeToSensor(2);
////    ZMQServer::subscribeToSensor(3);
////    ZMQServer::subscribeToSensor(4);
//    PointCloudGUI::launchRealTime();

    return 0;
}
