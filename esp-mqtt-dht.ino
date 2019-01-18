#include "config.h"

#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SimpleKalmanFilter.h>
#include <Wire.h>

String mqttServer;
String sensorTopic;
String clientName;
boolean mqttRetained = false;

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

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  pinMode(D0, OUTPUT);
  Serial.begin(115200);
  Serial.println();

  mqttServer = MQTT_SERVER;
  mqttRetained = MQTT_RETAINED;

  clientName = "esp8266-";
  clientName += MQTT_DEVICE_TYPE;
  clientName += "-";
  clientName += DEVICE_POSITION;
  clientName += "-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);

  sensorTopic = MQTT_BASE_TOPIC;
  sensorTopic += "/status/";
  sensorTopic += MQTT_DEVICE_TYPE;
  sensorTopic += "/";
  sensorTopic += DEVICE_POSITION;

  WiFi.hostname(clientName);
  setup_wifi();

  Serial.print("MQTT Server: ");
  Serial.println(mqttServer);
  Serial.print("MQTT Topic: ");
  Serial.println(sensorTopic);
  Serial.print("MQTT retained: ");
  Serial.println(mqttRetained ? "true" : "false");
  client.setServer(mqttServer.c_str(), 1883);
  client.setCallback(callback);

  dht.setup(DHTPIN, IS_DHT11 ? DHTesp::DHT11 : DHTesp::DHT22);
  Serial.print("DHT Sensor type: ");
  if (dht.getModel() == DHTesp::DHT22) {
    Serial.println("DHT22");
  } else if (dht.getModel() == DHTesp::DHT11) {
    Serial.println("DHT11");
  } else {
    Serial.println(dht.getModel());
  }
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
  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
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

    // Attempt to connect
    if (client.connect(clientName.c_str(), (sensorTopic + "/connected").c_str(), 0, mqttRetained, "0")) {
      Serial.println("MQTT connected");
      client.subscribe(String(sensorTopic + "/identify").c_str());
      Serial.println("MQTT subscribed identify");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  digitalWrite(D0, LOW); // Turn the LED on
}

void publish(const char* mqttTopic, float value, boolean retained) {
  Serial.print("publish ");
  Serial.print(mqttTopic);
  Serial.print(" ");
  Serial.print(value);
  Serial.print(" retained: ");
  Serial.println(retained);
  client.publish(mqttTopic, String(value).c_str(), retained);
}

void loop() {
  if (!client.connected()) {
    lastConnected = 0;
    reconnect();
  }

  digitalWrite(D0, HIGH); // Turn the LED off by making the voltage HIGH
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
    client.publish((sensorTopic + "/connected").c_str(), String(nextConnected).c_str(), mqttRetained);
  }

  if (readSuccessful) {
    float avgT = kalmanTemp.updateEstimate(t);
    sendTemp++;
    if (sendTemp >= SEND_EVERY_TEMP) {
      sendTemp = 0;
      publish((sensorTopic + "/temp").c_str(), avgT, mqttRetained);
    }
    Serial.print("Temperature in Celsius: ");
    Serial.print(String(t).c_str());
    Serial.print(" Average: ");
    Serial.println(String(avgT).c_str());

    float avgH = kalmanHum.updateEstimate(h);
    sendHum++;
    if (sendHum >= SEND_EVERY_HUM) {
      sendHum = 0;
      publish((sensorTopic + "/hum").c_str(), avgH, mqttRetained);
    }
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
    publish((sensorTopic + "/rssi").c_str(), avgRssi, mqttRetained);
  }
  Serial.print("RSSI        in dBm:     ");
  Serial.print(String(rssi).c_str());
  Serial.print("   Average: ");
  Serial.println(String(avgRssi).c_str());
}
