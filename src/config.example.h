#pragma once

#define WIFI_SSID "name"
#define WIFI_PASSWORD "password"

#define MQTT_SERVER "hostname"
#define MQTT_BASE_TOPIC "espDht-bed"
const bool MQTT_RETAINED = true;

const bool IS_DHT11 = false;

#ifdef ESP8266
  const int DHTPIN = 12; // digital pin of the NodeMCU v2 (D6 in this case)
#else // for ESP32
  const int DHTPIN = 15; // digital pin of the NodeMCU ESP32 (D15 in this case)
#endif

// hint: 1 measurement every 5 seconds, 12 measurements every minute
const int SEND_EVERY_TEMP = 12 * 2; // send every 2 minutes
const int SEND_EVERY_HUM = 12 * 5; // send every 5 minutes
const int SEND_EVERY_RSSI = 12 * 5; // send every 5 minutes

// Enable when you want to see the actual values published over MQTT after each measurement
// #define DEBUG_KALMAN
