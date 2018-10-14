#pragma once

#define SENSOR_NAME "bed"

#define WIFI_SSID "name"
#define WIFI_PASSWORD "password"

#define MQTT_SERVER "hostname"
// #define MQTT_USER "user"
// #define MQTT_PASSWORD "password"

#define MQTT_TOPIC_BASE "lightproject/status/temp/"
#define MQTT_TOPIC_SENSOR MQTT_TOPIC_BASE SENSOR_NAME

const boolean MQTT_RETAINED = true;
// increase the KeepAlive to 120s. Default is 15s
#define MQTT_KEEPALIVE 120

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11 // DHT 11
#define DHTTYPE DHT22 // DHT 22 (AM2302), AM2321
//#define DHTTYPE DHT21 // DHT 21 (AM2301)

const int DHTPIN = 12; // digital pin of the NodeMCU (D6 in this case)

// hint: 1 measurement every 5 seconds, 12 measurements every minute
const int SEND_EVERY_TEMP = 3; // send every 15 seconds
const int SEND_EVERY_HUM = 12; // send every 60 seconds
const int SEND_EVERY_RSSI = 12 * 10; // send every 10 minutes

// the time of the rolling average is 4 times higher than the time between two sends
// example: every 15 sends with a rolling average over 60 seconds
const int ROLLING_AVERAGE_BUFFER_SIZE_MULTIPLE_OF_SEND_FREQUENCY = 4;
