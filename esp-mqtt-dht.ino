#include "config.h"

#include <DHTesp.h>
#include <EspMQTTClient.h>
#include <SimpleKalmanFilter.h>
#include <Wire.h>

#define CLIENT_NAME MQTT_BASE_TOPIC "-" DEVICE_POSITION
#define SENSOR_TOPIC MQTT_BASE_TOPIC "/status/" DEVICE_POSITION

SimpleKalmanFilter kalmanTemp(0.2, 100, 0.01);
SimpleKalmanFilter kalmanHum(2, 100, 0.01);
SimpleKalmanFilter kalmanRssi(10, 1000, 0.01);

int lastConnected = 0;
const int SECONDS_BETWEEN_MEASURE = 5;
int currentCycle = 0;

int sendTemp = 0;
int sendHum = 0;
int sendRssi = 0;

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
  CLIENT_NAME,
  1883
);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial.println();

  Serial.print("Client Name: ");
  Serial.println(CLIENT_NAME);
  Serial.print("MQTT Topic: ");
  Serial.println(SENSOR_TOPIC);
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
  client.enableLastWillMessage(SENSOR_TOPIC "/connected", "0", MQTT_RETAINED);  // You can activate the retain flag by setting the third parameter to true
}

void onConnectionEstablished() {
  client.subscribe(SENSOR_TOPIC "/identify", [](const String & payload) {
    digitalWrite(LED_BUILTIN, LED_BUILTIN_ON); // Turn the LED on
  });

  client.publish(SENSOR_TOPIC "/connected", "1", MQTT_RETAINED);
  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
  lastConnected = 1;
}

void publish(const String &mqttTopic, float value, boolean retained) {
  Serial.print("publish ");
  Serial.print(mqttTopic);
  Serial.print(" ");
  Serial.print(value);
  Serial.print(" retained: ");
  Serial.println(retained);
  client.publish(mqttTopic, String(value), retained);
}

void loop() {
  if (!client.isConnected()) {
    lastConnected = 0;
    digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
  }

  client.loop();

  delay(1000);

  currentCycle++;
  if (currentCycle < SECONDS_BETWEEN_MEASURE) {
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
    client.publish(SENSOR_TOPIC "/connected", String(nextConnected), MQTT_RETAINED);
  }

  if (readSuccessful) {
    float avgT = kalmanTemp.updateEstimate(t);
    sendTemp++;
    if (sendTemp >= SEND_EVERY_TEMP) {
      sendTemp = 0;
      publish(SENSOR_TOPIC "/temp", avgT, MQTT_RETAINED);
    }
#ifdef DEBUG_KALMAN
    publish(SENSOR_TOPIC "-orig/temp", t, MQTT_RETAINED);
    publish(SENSOR_TOPIC "-avg/temp", avgT, MQTT_RETAINED);
#endif
    Serial.print("Temperature in Celsius: ");
    Serial.print(String(t).c_str());
    Serial.print(" Average: ");
    Serial.println(String(avgT).c_str());

    float avgH = kalmanHum.updateEstimate(h);
    sendHum++;
    if (sendHum >= SEND_EVERY_HUM) {
      sendHum = 0;
      publish(SENSOR_TOPIC "/hum", avgH, MQTT_RETAINED);
    }
#ifdef DEBUG_KALMAN
    publish(SENSOR_TOPIC "-orig/hum", h, MQTT_RETAINED);
    publish(SENSOR_TOPIC "-avg/hum", avgH, MQTT_RETAINED);
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
  float avgRssi = kalmanRssi.updateEstimate(rssi);
  sendRssi++;
  if (sendRssi >= SEND_EVERY_RSSI) {
    sendRssi = 0;
    publish(SENSOR_TOPIC "/rssi", avgRssi, MQTT_RETAINED);
  }
#ifdef DEBUG_KALMAN
  publish(SENSOR_TOPIC "-orig/rssi", rssi, MQTT_RETAINED);
  publish(SENSOR_TOPIC "-avg/rssi", avgRssi, MQTT_RETAINED);
#endif
  Serial.print("RSSI        in dBm:     ");
  Serial.print(String(rssi).c_str());
  Serial.print("   Average: ");
  Serial.println(String(avgRssi).c_str());
}
