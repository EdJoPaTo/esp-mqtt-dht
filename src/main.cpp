#include <credentials.h>
#include <DHTesp.h>
#include <EspMQTTClient.h>
#include <MqttKalmanPublish.h>
#include <Wire.h>

#define CLIENT_NAME "espDht-location"
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

#define BASE_TOPIC CLIENT_NAME "/"
#define BASE_TOPIC_STATUS BASE_TOPIC "status/"
#define BASE_TOPIC_SET BASE_TOPIC "set/"

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
  MQTT_USERNAME,
  MQTT_PASSWORD,
  CLIENT_NAME,
  1883
);

MQTTKalmanPublish mkTemp(client, BASE_TOPIC_STATUS "temp", MQTT_RETAINED, SEND_EVERY_TEMP, 0.2);
MQTTKalmanPublish mkHum(client, BASE_TOPIC_STATUS "hum", MQTT_RETAINED, SEND_EVERY_HUM, 2);
MQTTKalmanPublish mkRssi(client, BASE_TOPIC_STATUS "rssi", MQTT_RETAINED, SEND_EVERY_RSSI, 10);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial.println();

  Serial.print("MQTT Client Name: ");
  Serial.println(CLIENT_NAME);
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

  client.enableDebuggingMessages();
  client.enableHTTPWebUpdater();
  client.enableOTA();
  client.enableLastWillMessage(BASE_TOPIC "connected", "0", MQTT_RETAINED);
}

void onConnectionEstablished() {
  client.subscribe(BASE_TOPIC_SET "identify", [](const String & payload) {
    digitalWrite(LED_BUILTIN, LED_BUILTIN_ON); // Turn the LED on
  });

  client.publish(BASE_TOPIC "git-version", GIT_VERSION, MQTT_RETAINED);
  lastConnected = 1;
  client.publish(BASE_TOPIC "connected", String(lastConnected), MQTT_RETAINED);
}

void loop() {
  client.loop();
  digitalWrite(LED_BUILTIN, client.isConnected() ? LED_BUILTIN_OFF : LED_BUILTIN_ON);

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
  if (client.isConnected()) {
    int nextConnected = readSuccessful ? 2 : 1;
    if (nextConnected != lastConnected) {
      Serial.printf("set /connected from %d to %d\n", lastConnected, nextConnected);
      lastConnected = nextConnected;
      client.publish(BASE_TOPIC "connected", String(nextConnected), MQTT_RETAINED);
    }
  }

  if (readSuccessful) {
    float avgT = mkTemp.addMeasurement(t);
#ifdef DEBUG_KALMAN
    client.publish(BASE_TOPIC_STATUS "orig/temp", String(t), MQTT_RETAINED);
    client.publish(BASE_TOPIC_STATUS "avg/temp", String(avgT), MQTT_RETAINED);
#endif
    Serial.printf("Temperature in Celsius: %5.1f Average: %6.2f\n", t, avgT);

    float avgH = mkHum.addMeasurement(h);
#ifdef DEBUG_KALMAN
    client.publish(BASE_TOPIC_STATUS "orig/hum", String(h), MQTT_RETAINED);
    client.publish(BASE_TOPIC_STATUS "avg/hum", String(avgH), MQTT_RETAINED);
#endif
    Serial.printf("Humidity    in Percent: %5.1f Average: %6.2f\n", h, avgH);
  } else {
    Serial.print("Failed to read from sensor! ");
    Serial.println(dht.getStatusString());
  }

  long rssi = WiFi.RSSI();
  float avgRssi = mkRssi.addMeasurement(rssi);
#ifdef DEBUG_KALMAN
  client.publish(BASE_TOPIC_STATUS "orig/rssi", String(rssi), MQTT_RETAINED);
  client.publish(BASE_TOPIC_STATUS "avg/rssi", String(avgRssi), MQTT_RETAINED);
#endif
  Serial.printf("RSSI        in     dBm: %3ld   Average: %6.2f\n", rssi, avgRssi);
}
