#pragma once

#include <EspMQTTClient.h>
#include <SimpleKalmanFilter.h>

class MQTTKalmanPublish
{
private:
    EspMQTTClient& client;
    const char* topic;
    const bool retained;

    const int sendEveryN;
    int currentCount = 0;

    SimpleKalmanFilter kalman;
public:
    MQTTKalmanPublish(
        EspMQTTClient& client,
        const char* topic,
        const bool retained,
        const float kalmanSensitivity,
        const int sendEveryN
    );

    float addMeasurement(float value);
};
