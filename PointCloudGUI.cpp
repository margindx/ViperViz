//
// Created by Hamza El-Kebir on 2/11/25.
//

#include "PointCloudGUI.hpp"

Color PointCloudGUI::sensorColors[8] = {RED, GREEN, BLUE, PURPLE,
                                        MAROON, LIME, SKYBLUE, VIOLET };

bool PointCloudGUI::recordingPoints = false;
bool PointCloudGUI::doBuffer = true;
bool PointCloudGUI::doDrawLine = false;

void PointCloudGUI::drawSensorData(SensorData &data, double r, double l) {
    Vector3 pos = {data.x, data.z, data.y};
    Vector3 nor = {data.nx, data.nz, data.ny};

    DrawSphere(pos, r, sensorColors[data.sensor_id % 8]);
    if (doDrawLine) DrawLine3D(pos, Vector3Add(pos, Vector3Scale(nor, l)), BLACK);
}

int PointCloudGUI::launch() {
    int screenWidth = 800;
    int screenHeight = 450;

    raylib::Window window(screenWidth, screenHeight, "raylib-cpp - basic window");
//        raylib::Texture logo("raylib_logo.png");

    SetTargetFPS(60);

    raylib::Camera3D camera;
    camera.position = Vector3{10, 10, 10};
    camera.target = Vector3{};
    camera.up = Vector3{0, 1, 0};
    camera.fovy = 45;
    camera.projection = CAMERA_PERSPECTIVE;

    Vector3 cubePosition{};

    DisableCursor();

    SensorData dataSample = {2, 2, 2, 2, 2, 1, 1, 1};
    SensorData::normalizeNormal(dataSample);

    while (!window.ShouldClose())
    {
        UpdateCamera(&camera, CAMERA_THIRD_PERSON);

        if (IsKeyPressed('Z')) camera.position = (Vector3){ 10, 10, 10 };

        if (IsKeyPressed('P')) recordingPoints = !recordingPoints;

        BeginDrawing();

        window.ClearBackground(RAYWHITE);

        BeginMode3D(camera);

//        DrawCube(cubePosition, 2, 2, 2, RED);
//        DrawCubeWires(cubePosition, 2, 2, 2, MAROON);

        drawSensorData(dataSample);

        DrawGrid(20, 1);

        EndMode3D();

        EndDrawing();
    }

    // UnloadTexture() and CloseWindow() are called automatically.

    return 0;
}

int PointCloudGUI::launchRealTime() {
    int screenWidth = 800;
    int screenHeight = 450;

    raylib::Window window(screenWidth, screenHeight, "EM Tracker Point Cloud Viz");
//        raylib::Texture logo("raylib_logo.png");

    SetTargetFPS(60);

    raylib::Camera3D camera;
    camera.position = Vector3{2, 2, 2};
    camera.target = Vector3{};
    camera.up = Vector3{0, 1, 0};
    camera.fovy = 45;
    camera.projection = CAMERA_PERSPECTIVE;

    Vector3 cubePosition{};

    DisableCursor();

    SensorData dataSample = {2, 2, 2, 2, 2, 1, 1, 1};
    SensorData::normalizeNormal(dataSample);

    while (!window.ShouldClose())
    {
        UpdateCamera(&camera, CAMERA_THIRD_PERSON);

        if (IsKeyPressed('Z')) camera.position = (Vector3){ 2, 2, 2 };

        if (IsKeyPressed('P')) {
            recordingPoints = !recordingPoints;
            std::cout << "Point recording: " << (recordingPoints ? "ON" : "OFF") << std::endl;
        }

        if (IsKeyPressed('B')) {
            doBuffer = !doBuffer;
            std::cout << "Buffering: " << (doBuffer ? "ON" : "OFF") << std::endl;
        }

        if (IsKeyPressed('L')) {
            doDrawLine = !doDrawLine;
            std::cout << "Line drawing: " << (doDrawLine ? "ON" : "OFF") << std::endl;
        }

        if (IsKeyPressed('R')) {
            ZMQServer::TopicSensorData topicSensorData = ZMQServer::receiveSensorData();
            std::string received_topic = topicSensorData.topic;
            SensorData received_data = topicSensorData.data;

            camera.target = Vector3{received_data.x, received_data.z, received_data.y};
            std::cout << "Reset camera target." << std::endl;
        }

        BeginDrawing();

        window.ClearBackground(RAYWHITE);

        BeginMode3D(camera);

//        DrawCube(cubePosition, 2, 2, 2, RED);
//        DrawCubeWires(cubePosition, 2, 2, 2, MAROON);

        if (recordingPoints) {
            ZMQServer::TopicSensorData topicSensorData = ZMQServer::receiveSensorData();
            std::string received_topic = topicSensorData.topic;
            SensorData received_data = topicSensorData.data;

            // Print received data
            std::cout << "Received Topic: " << received_topic
                      << ", Sensor ID: " << received_data.sensor_id
                      << ", t: " << longTimeStampToIso8601(received_data.time)
                      << ", p: (" << received_data.x << ", " << received_data.y << ", " << received_data.z << ")"
                      << ", n: (" << received_data.nx << ", " << received_data.ny << ", " << received_data.nz << ")" << std::endl;

            drawSensorData(received_data);

            for (auto& sensor: ZMQServer::sensorRcvMap) {
                for (auto& data: sensor.second) {
                    drawSensorData(data);
                }
            }
        }

//        drawSensorData(dataSample);

        DrawGrid(20, 0.1);

        EndMode3D();

        EndDrawing();
    }

    // UnloadTexture() and CloseWindow() are called automatically.

    return 0;
}
