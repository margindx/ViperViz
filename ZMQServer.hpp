//
// Created by Hamza El-Kebir on 2/7/25.
//

#ifndef VIPER_ZMQSERVER_HPP
#define VIPER_ZMQSERVER_HPP

#include <zmq.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include <random>

#include <deque>
#include <unordered_map>

#include "SensorData.hpp"

#define SENSOR_BUFF_LEN 15
#define SENSOR_RCV_BUFF_LEN 500

// Function to serialize SensorData to a string (for sending over ZMQ)
std::string serializeSensorData(const SensorData& data);

// Function to deserialize SensorData from a string (received from ZMQ)
SensorData deserializeSensorData(const std::string& data);

std::string longTimeStampToIso8601(long long timestamp_ms);

class ZMQServer {
protected:
    static zmq::context_t _context;
    static zmq::socket_t _publisher;
    static zmq::socket_t _subscriber;
    static int _port;
    static int _hwm;
public:
    static std::deque<SensorData> sensorSndBuffer;
    static std::unordered_map<std::string, std::deque<SensorData>> sensorRcvMap;
    static bool verbose;

    enum Role {
        Publisher,
        Subscriber,
        Both
    };

    struct TopicSensorData {
        std::string topic;
        SensorData data;
    };

    static int serveSimple();

    static int serve();

    static int consume();

    static int log();

    static bool setPort(int port, Role role=Both);

    static bool subscribeTo(std::string topic);

    static bool subscribeToSensor(int sensor_id);

    static bool unsubscribeFrom(std::string topic);

    static bool unsubscribeFromSensor(int sensor_id);

    static void init(int port, Role role=Both);

    static void publishSensorData(SensorData data);

    static TopicSensorData receiveSensorData(bool doBuffer=true);
};


#endif //VIPER_ZMQSERVER_HPP
