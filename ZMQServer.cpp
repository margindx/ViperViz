//
// Created by Hamza El-Kebir on 2/7/25.
//

#include "ZMQServer.hpp"
#include <cmath>
#include <thread>

zmq::context_t ZMQServer::_context;
zmq::socket_t ZMQServer::_publisher;
zmq::socket_t ZMQServer::_subscriber;
int ZMQServer::_port = 5555;
int ZMQServer::_hwm = 1;

std::deque<SensorData> ZMQServer::sensorSndBuffer = {};
std::unordered_map<std::string, std::deque<SensorData>> ZMQServer::sensorRcvMap = {};
bool ZMQServer::verbose = true;

std::string serializeSensorData(const SensorData &data) {
    std::stringstream ss;
    ss.write(reinterpret_cast<const char*>(&data), sizeof(SensorData));
    return ss.str();
}

SensorData deserializeSensorData(const std::string &data) {
    SensorData sensorData{};
    std::stringstream ss(data);
    ss.read(reinterpret_cast<char*>(&sensorData), sizeof(SensorData));
    return sensorData;
}

std::string longTimeStampToIso8601(long long int timestamp_ms) {
    using namespace std::chrono;

    // Convert milliseconds to time_point
    auto time_point = time_point_cast<milliseconds>(
        system_clock::time_point(duration_cast<milliseconds>(
            std::chrono::milliseconds(timestamp_ms)
        ))
    );

    // Convert time_point to time_t
    std::time_t time_t_value = system_clock::to_time_t(time_point);

    // Format time_t to ISO 8601 with milliseconds
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_value), "%Y-%m-%dT%H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << timestamp_ms % 1000 << "Z";

    return ss.str();
}

int ZMQServer::serveSimple() {
    // Random number generation for sensor data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-10.0, 10.0);
    int sensor_id_counter = 2;

    if (verbose) ::std::cout << "Starting to serve test data..." << ::std::endl;

    while (true) {
        // Create some sample sensor data
        SensorData data{};
        data.sensor_id = sensor_id_counter;
        data.time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        data.x = dis(gen);
        data.y = dis(gen);
        data.z = dis(gen);

        data.nx = dis(gen);
        data.ny = dis(gen);
        data.nz = dis(gen);

        SensorData::normalizeNormal(data);

        publishSensorData(data);

        if (verbose) std::cout << "Published sensor data: " << data.sensor_id
                               << ", t: " << longTimeStampToIso8601(data.time)
                               << ", p: (" << data.x << ", " << data.y << ", " << data.z << ")"
                               << ", n: (" << data.nx << ", " << data.ny << ", " << data.nz << ")" << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Send data every 0.1 seconds
    }

    return 0;
}

int ZMQServer::serve() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> acc_dis(-1.0, 1.0);
    int sensor_id_counter = 2;

    // Initial conditions
    SensorDataExtended data{};
    data.sensor_id = sensor_id_counter;
    data.x = data.y = data.z = 0.0;
    data.nx = 0.0; data.ny = 0.0; data.nz = -1.0;
    data.vx = data.vy = data.vz = 0.0;
    data.vnx = data.vny = data.vnz = 0.0;

    const double dt = 0.001; // Time step in seconds

    if (verbose) std::cout << "Starting to serve test data..." << std::endl;

    while (true) {
        // Generate random accelerations
        double ax = 10*acc_dis(gen);
        double ay = 10*acc_dis(gen);
        double az = 10*acc_dis(gen);
        double anx = 5*acc_dis(gen);
        double any = 5*acc_dis(gen);
        double anz = 5*acc_dis(gen);

        // Integrate velocities
        data.vx += ax * dt;
        data.vy += ay * dt;
        data.vz += az * dt;
        data.vnx += anx * dt;
        data.vny += any * dt;
        data.vnz += anz * dt;

        // Integrate positions
        data.x += data.vx * dt;
        data.y += data.vy * dt;
        data.z += data.vz * dt;
        data.nx += data.vnx * dt;
        data.ny += data.vny * dt;
        data.nz += data.vnz * dt;

        SensorDataExtended::normalizeNormal(data);

        // Update timestamp
        data.time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
        ).count();

        auto dataFinal = data.convertV();
        // Clamp position
        dataFinal.x = fmin(fmax(-1, dataFinal.x), 1);
        dataFinal.y = fmin(fmax(-1, dataFinal.y), 1);
        dataFinal.z = fmin(fmax( 0, dataFinal.z), 1);

        publishSensorData(dataFinal);

        if (verbose) std::cout << "Published sensor data: " << data.sensor_id
                               << ", t: " << longTimeStampToIso8601(data.time)
                               << ", p: (" << data.x << ", " << data.y << ", " << data.z << ")"
                               << ", n: (" << data.nx << ", " << data.ny << ", " << data.nz << ")"
                               << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return 0;
}




int ZMQServer::consume() {
    while (true) {
        // Receive subscribed SensorData
        TopicSensorData topicSensorData = receiveSensorData();
        std::string received_topic = topicSensorData.topic;
        SensorData received_data = topicSensorData.data;

        // Print received data
        if (verbose) std::cout << "Received Topic: " << received_topic
                               << ", Sensor ID: " << received_data.sensor_id
                               << ", t: " << longTimeStampToIso8601(received_data.time)
                               << ", p: (" << received_data.x << ", " << received_data.y << ", " << received_data.z << ")"
                               << ", n: (" << received_data.nx << ", " << received_data.ny << ", " << received_data.nz << ")" << std::endl;
    }

    return 0;
}

bool ZMQServer::setPort(int port, ZMQServer::Role role) {
    _port = port;

    if (role == Publisher || role == Both) {
        ::std::string publisher_addr = "tcp://*:" + ::std::to_string(port);
        _publisher.bind(publisher_addr);
        if (verbose) {
            std::cout << "Publisher started, publishing on " << publisher_addr << std::endl;
        }
    }

    if (role == Subscriber || role == Both) {
        ::std::string subscriber_addr = "tcp://localhost:" + ::std::to_string(port);
        _subscriber.connect(subscriber_addr);
        if (verbose) {
            ::std::cout << "Subscriber started on " << subscriber_addr << ::std::endl;
        }
    }

    return true;
}

bool ZMQServer::subscribeTo(std::string topic) {
    _subscriber.set(zmq::sockopt::subscribe, topic);
    if (verbose) ::std::cout << "Subscribed to topic: " << topic << ::std::endl;

    return true;
}

bool ZMQServer::subscribeToSensor(int sensor_id) {
    std::string topic = "sensor/" + std::to_string(sensor_id);

    return subscribeTo(topic);
}

bool ZMQServer::unsubscribeFrom(std::string topic) {
    _subscriber.set(zmq::sockopt::unsubscribe, topic);
    if (verbose) ::std::cout << "Unsubscribed from topic: " << topic << ::std::endl;

    return true;
}

bool ZMQServer::unsubscribeFromSensor(int sensor_id) {
    std::string topic = "sensor/" + std::to_string(sensor_id);

    return unsubscribeFrom(topic);
}

void ZMQServer::init(int port, ZMQServer::Role role) {
    // ZeroMQ context
    _context = zmq::context_t{1};

    // Publisher socket
    _publisher = zmq::socket_t{_context, zmq::socket_type::pub};
    _publisher.set(zmq::sockopt::sndhwm, _hwm);

    // Subscriber socket
    _subscriber = zmq::socket_t{_context, zmq::socket_type::sub};
    _subscriber.set(zmq::sockopt::rcvhwm, _hwm);

    setPort(port, role);
}

void ZMQServer::publishSensorData(SensorData data) {
    // Maintain FIFO buffer
    if (sensorSndBuffer.size() >= SENSOR_BUFF_LEN) {
        sensorSndBuffer.pop_front();
    }
    sensorSndBuffer.push_back(data);

    // Determine topic based on sensor_id (example: sensor/1, sensor/2, etc.)
    std::string topic = "sensor/" + std::to_string(data.sensor_id);
//    std::string topic = "sensor/1";

    // Serialize the SensorData
    std::string data_str = serializeSensorData(data);

    // Create a multipart message: [topic][data]
    zmq::message_t topic_msg(topic.size());
    memcpy(topic_msg.data(), topic.data(), topic.size());

    zmq::message_t data_msg(data_str.size());
    memcpy(data_msg.data(), data_str.data(), data_str.size());

    // Send the topic
    _publisher.send(topic_msg, zmq::send_flags::sndmore);

    // Send the serialized SensorData
    _publisher.send(data_msg, zmq::send_flags::none);

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

ZMQServer::TopicSensorData ZMQServer::receiveSensorData(bool doBuffer) {
    // Receive topic
    zmq::message_t topic_msg;
    _subscriber.recv(topic_msg, zmq::recv_flags::none);
    std::string received_topic = std::string(static_cast<char*>(topic_msg.data()), topic_msg.size());

    // Receive data
    zmq::message_t data_msg;
    _subscriber.recv(data_msg, zmq::recv_flags::none);
    std::string received_data_str = std::string(static_cast<char*>(data_msg.data()), data_msg.size());

    // Deserialize SensorData
    SensorData received_data = deserializeSensorData(received_data_str);

    // Maintain FIFO buffer for each topic
    if (doBuffer) {
        auto &data_queue = sensorRcvMap[received_topic];
        if (data_queue.size() >= SENSOR_RCV_BUFF_LEN) {
            data_queue.pop_front();
        }
//    std::cout << "Buffer for topic " << received_topic << " has length " << data_queue.size() << std::endl;
        data_queue.push_back(received_data);
    }

    return {received_topic, received_data};
}

int ZMQServer::log() {
    while (true) {
        // Receive subscribed SensorData
        TopicSensorData topicSensorData = receiveSensorData();
        std::string received_topic = topicSensorData.topic;
        SensorData received_data = topicSensorData.data;

        std::cout << std::fixed << std::setprecision(12)
                  << received_data.time << "," << received_data.sensor_id << ","
                  << received_data.x << "," << received_data.y << "," << received_data.z << ","
                  << received_data.qx << "," << received_data.qy << "," << received_data.qz << "," << received_data.qw << std::endl;
//                  << received_data.nx << "," << received_data.ny << "," << received_data.nz << std::endl;
    }

    return 0;
}
