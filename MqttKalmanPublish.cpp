#include "MqttKalmanPublish.h"

const float INITIAL_ESTIMATION_ERROR = 1000;
const float INITIAL_Q = 0.02;

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
    kalman(kalmanSensitivity, INITIAL_ESTIMATION_ERROR, INITIAL_Q)
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

void MQTTKalmanPublish::restart() {
    kalman.setEstimateError(INITIAL_ESTIMATION_ERROR);
    currentCount = 0;
}
