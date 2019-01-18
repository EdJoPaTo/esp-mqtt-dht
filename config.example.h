#pragma once

const String DEVICE_POSITION = "bed";

#define WIFI_SSID "name"
#define WIFI_PASSWORD "password"

const String MQTT_SERVER = "hostname";
const String MQTT_BASE_TOPIC = "lightproject";
const String MQTT_DEVICE_TYPE = "temp";
const boolean MQTT_RETAINED = true;

const boolean IS_DHT11 = false;
const int DHTPIN = 12; // digital pin of the NodeMCU (D6 in this case)

// hint: 1 measurement every 5 seconds, 12 measurements every minute
const int SEND_EVERY_TEMP = 12; // send every 60 seconds
const int SEND_EVERY_HUM = 12 * 5; // send every 5 minutes
const int SEND_EVERY_RSSI = 12 * 15; // send every 15 minutes
