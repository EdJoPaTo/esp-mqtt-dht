#include "config.h"

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include "DHT.h"

// the following consts are calculated for easier use while program
const int ROLLING_AVERAGE_BUFFER_SIZE_TEMP = ROLLING_AVERAGE_BUFFER_SIZE_MULTIPLE_OF_SEND_FREQUENCY * SEND_EVERY_TEMP;
const int ROLLING_AVERAGE_BUFFER_SIZE_HUM = ROLLING_AVERAGE_BUFFER_SIZE_MULTIPLE_OF_SEND_FREQUENCY * SEND_EVERY_HUM;
const int ROLLING_AVERAGE_BUFFER_SIZE_RSSI = ROLLING_AVERAGE_BUFFER_SIZE_MULTIPLE_OF_SEND_FREQUENCY * SEND_EVERY_RSSI;

float rollingBufferTemperature[ROLLING_AVERAGE_BUFFER_SIZE_TEMP];
float rollingBufferHumidity[ROLLING_AVERAGE_BUFFER_SIZE_HUM];
float rollingBufferRssi[ROLLING_AVERAGE_BUFFER_SIZE_RSSI];
int rollingBufferTempCurrentIndex = 0;
int rollingBufferTempCurrentSize = 0;
int rollingBufferHumCurrentIndex = 0;
int rollingBufferHumCurrentSize = 0;
int rollingBufferRssiCurrentIndex = 0;
int rollingBufferRssiCurrentSize = 0;

int lastConnected = 0;

DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client.setServer(MQTT_SERVER, 1883);
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.hostname("esp-temp-" SENSOR_NAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");

    // Generate client name based on MAC address and last 8 bits of microsecond counter
    String clientName;
    clientName += "esp8266-";
    clientName += SENSOR_NAME "-";
    uint8_t mac[6];
    WiFi.macAddress(mac);
    clientName += macToStr(mac);
    clientName += "-";
    clientName += String(micros() & 0xff, 16);
    Serial.print("Connecting to ");
    Serial.print(MQTT_SERVER);
    Serial.print(" as ");
    Serial.println(clientName);


    // Attempt to connect
#ifdef MQTT_USER
    if (client.connect((char*) clientName.c_str(), MQTT_USER, MQTT_PASSWORD, MQTT_TOPIC_SENSOR "/connected", 0, MQTT_RETAINED, "0")) {
#else
    if (client.connect((char*) clientName.c_str(), MQTT_TOPIC_SENSOR "/connected", 0, MQTT_RETAINED, "0")) {
#endif
      Serial.println("MQTT connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

float calculateRollingAverage(int currentSize, int rollingBufferSize, float rollingBuffer[], int currentIndex) {
  float sum = 0;

  for (size_t i = 0; i < currentSize; i++) {
    int index = (rollingBufferSize + currentIndex - i) % rollingBufferSize;
    sum += rollingBuffer[index];
  }

  return sum / currentSize;
}

float handleNewSensorValue(float sensorValue, float buffer[], int *bufferCurrentSize, int *bufferCurrentIndex, int bufferTotalSize, int sendFrequency, const char* mqttTopic, boolean retained) {
  if (*bufferCurrentSize < bufferTotalSize) {
    *bufferCurrentSize += 1;
  }

/*
  Serial.print("current Index: ");
  Serial.print(*bufferCurrentIndex);
  Serial.print(" size: ");
  Serial.print(*bufferCurrentSize);
  Serial.print(" total: ");
  Serial.println(bufferTotalSize);
/**/

  buffer[*bufferCurrentIndex] = sensorValue;
  float avg = calculateRollingAverage(*bufferCurrentSize, bufferTotalSize, buffer, *bufferCurrentIndex);

  if (*bufferCurrentIndex % sendFrequency == sendFrequency - 1) {
    Serial.print("publish ");
    Serial.print(mqttTopic);
    Serial.print(" ");
    Serial.print(avg);
    Serial.print(" retained: ");
    Serial.println(retained);
    client.publish(mqttTopic, String(avg).c_str(), retained);
  }

  *bufferCurrentIndex += 1;
  *bufferCurrentIndex %= bufferTotalSize;

  return avg;
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Wait a few seconds between measurements.
  delay(5000);

  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  boolean readSuccessful = !isnan(t) && !isnan(h);
  int nextConnected = readSuccessful ? 2 : 1;

  if (nextConnected != lastConnected) {
    Serial.print("set /connected from ");
    Serial.print(lastConnected);
    Serial.print(" to ");
    Serial.println(nextConnected);
    lastConnected = nextConnected;
    client.publish(MQTT_TOPIC_SENSOR "/connected", String(nextConnected).c_str(), MQTT_RETAINED);
  }

  if (readSuccessful) {
    float avgT = handleNewSensorValue(t, rollingBufferTemperature, &rollingBufferTempCurrentSize, &rollingBufferTempCurrentIndex, ROLLING_AVERAGE_BUFFER_SIZE_TEMP, SEND_EVERY_TEMP, MQTT_TOPIC_SENSOR "/temp", MQTT_RETAINED);
    Serial.print("Temperature in Celsius: ");
    Serial.print(String(t).c_str());
    Serial.print(" Average: ");
    Serial.println(String(avgT).c_str());

    float avgH = handleNewSensorValue(h, rollingBufferHumidity, &rollingBufferHumCurrentSize, &rollingBufferHumCurrentIndex, ROLLING_AVERAGE_BUFFER_SIZE_HUM, SEND_EVERY_HUM, MQTT_TOPIC_SENSOR "/hum", MQTT_RETAINED);
    Serial.print("Humidity    in Percent: ");
    Serial.print(String(h).c_str());
    Serial.print(" Average: ");
    Serial.println(String(avgH).c_str());
  } else {
    Serial.println("Failed to read from sensor!");
    if (rollingBufferTempCurrentSize > 0) {
      rollingBufferTempCurrentSize--;
    }
    if (rollingBufferHumCurrentSize > 0) {
      rollingBufferHumCurrentSize--;
    }
  }

  long rssi = WiFi.RSSI();
  float avgRssi = handleNewSensorValue(rssi, rollingBufferRssi, &rollingBufferRssiCurrentSize, &rollingBufferRssiCurrentIndex, ROLLING_AVERAGE_BUFFER_SIZE_RSSI, SEND_EVERY_RSSI, MQTT_TOPIC_SENSOR "/rssi", false);
  Serial.print("RSSI        in dBm:     ");
  Serial.print(String(rssi).c_str());
  Serial.print("   Average: ");
  Serial.println(String(avgRssi).c_str());
}
