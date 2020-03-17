#include "MqttKalmanPublish.h"

MQTTKalmanPublish::MQTTKalmanPublish(
    EspMQTTClient& client,
    const char* topic,
    const bool retained,
    const float kalmanSensitivity,
    const int sendEveryN
) :
    client(client),
    topic(topic),
    retained(retained),
    sendEveryN(sendEveryN),
    kalman(kalmanSensitivity, 1000, 0.01)
{ }

float MQTTKalmanPublish::addMeasurement(float value) {
    float estimate = kalman.updateEstimate(value);

    currentCount++;
    if (currentCount >= sendEveryN) {
        currentCount = 0;
        client.publish(topic, String(estimate), retained);
    }

    return estimate;
}
