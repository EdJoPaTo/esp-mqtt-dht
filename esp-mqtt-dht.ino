#include "config.h"

#include <DHTesp.h>
#include <EspMQTTClient.h>
#include <SimpleKalmanFilter.h>
#include <Wire.h>

#include "MqttKalmanPublish.h"

#define SENSOR_TOPIC MQTT_BASE_TOPIC "/status"
#define SENSOR_SET_TOPIC MQTT_BASE_TOPIC "/set"

int lastConnected = 0;
const int SECONDS_BETWEEN_MEASURE = 5;
const int MQTT_UPDATES_PER_SECOND = 1;
const int INTERVALS = SECONDS_BETWEEN_MEASURE * MQTT_UPDATES_PER_SECOND;
int currentCycle = 0;

DHTesp dht;

#ifdef ESP8266
  #define LED_BUILTIN_ON LOW
  #define LED_BUILTIN_OFF HIGH
#else // for ESP32
  #define LED_BUILTIN_ON HIGH
  #define LED_BUILTIN_OFF LOW
#endif

EspMQTTClient client(
  WIFI_SSID,
  WIFI_PASSWORD,
  MQTT_SERVER,
  MQTT_BASE_TOPIC,
  1883
);

MQTTKalmanPublish mkTemp(client, SENSOR_TOPIC "/temp", MQTT_RETAINED, 0.2, SEND_EVERY_TEMP);
MQTTKalmanPublish mkHum(client, SENSOR_TOPIC "/hum", MQTT_RETAINED, 2, SEND_EVERY_HUM);
MQTTKalmanPublish mkRssi(client, SENSOR_TOPIC "/rssi", MQTT_RETAINED, 10, SEND_EVERY_RSSI);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial.println();

  Serial.print("MQTT Base Topic: ");
  Serial.println(MQTT_BASE_TOPIC);
  Serial.print("MQTT retained: ");
  Serial.println(MQTT_RETAINED ? "true" : "false");

  dht.setup(DHTPIN, IS_DHT11 ? DHTesp::DHT11 : DHTesp::DHT22);
  Serial.print("DHT Sensor type: ");
  if (dht.getModel() == DHTesp::DHT22) {
    Serial.println("DHT22");
  } else if (dht.getModel() == DHTesp::DHT11) {
    Serial.println("DHT11");
  } else {
    Serial.println(dht.getModel());
  }

  // Optional functionnalities of EspMQTTClient
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  client.enableLastWillMessage(MQTT_BASE_TOPIC "/connected", "0", MQTT_RETAINED);  // You can activate the retain flag by setting the third parameter to true
}

void onConnectionEstablished() {
  client.subscribe(SENSOR_SET_TOPIC "/identify", [](const String & payload) {
    digitalWrite(LED_BUILTIN, LED_BUILTIN_ON); // Turn the LED on
  });

  client.publish(MQTT_BASE_TOPIC "/connected", "1", MQTT_RETAINED);
  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
  lastConnected = 1;
}

void loop() {
  if (!client.isConnected()) {
    lastConnected = 0;
    digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
  }

  client.loop();
  digitalWrite(LED_BUILTIN, client.isConnected() ? LED_BUILTIN_OFF : LED_BUILTIN_OFF);

  delay(1000 / MQTT_UPDATES_PER_SECOND);

  currentCycle++;
  if (currentCycle < INTERVALS) {
    return;
  }
  currentCycle = 0;

  // Read temperature as Celsius (the default)
  float t = dht.getTemperature();
  float h = dht.getHumidity();

  boolean readSuccessful = dht.getStatus() == DHTesp::ERROR_NONE;
  int nextConnected = readSuccessful ? 2 : 1;

  if (nextConnected != lastConnected) {
    Serial.print("set /connected from ");
    Serial.print(lastConnected);
    Serial.print(" to ");
    Serial.println(nextConnected);
    lastConnected = nextConnected;
    client.publish(MQTT_BASE_TOPIC "/connected", String(nextConnected), MQTT_RETAINED);
  }

  if (readSuccessful) {
    float avgT = mkTemp.addMeasurement(t);
#ifdef DEBUG_KALMAN
    client.publish(SENSOR_TOPIC "-orig/temp", String(t), MQTT_RETAINED);
    client.publish(SENSOR_TOPIC "-avg/temp", String(avgT), MQTT_RETAINED);
#endif
    Serial.print("Temperature in Celsius: ");
    Serial.print(String(t).c_str());
    Serial.print(" Average: ");
    Serial.println(String(avgT).c_str());

    float avgH = mkHum.addMeasurement(h);
#ifdef DEBUG_KALMAN
    client.publish(SENSOR_TOPIC "-orig/hum", String(h), MQTT_RETAINED);
    client.publish(SENSOR_TOPIC "-avg/hum", String(avgH), MQTT_RETAINED);
#endif
    Serial.print("Humidity    in Percent: ");
    Serial.print(String(h).c_str());
    Serial.print(" Average: ");
    Serial.println(String(avgH).c_str());
  } else {
    Serial.print("Failed to read from sensor! ");
    Serial.println(dht.getStatusString());
  }

  long rssi = WiFi.RSSI();
  float avgRssi = mkRssi.addMeasurement(rssi);
#ifdef DEBUG_KALMAN
  client.publish(SENSOR_TOPIC "-orig/rssi", String(rssi), MQTT_RETAINED);
  client.publish(SENSOR_TOPIC "-avg/rssi", String(avgRssi), MQTT_RETAINED);
#endif
  Serial.print("RSSI        in dBm:     ");
  Serial.print(String(rssi).c_str());
  Serial.print("   Average: ");
  Serial.println(String(avgRssi).c_str());
}
