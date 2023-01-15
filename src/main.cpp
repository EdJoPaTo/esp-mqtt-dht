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
const unsigned long MEASURE_EVERY_N_MILLIS = 5 * 1000; // every 5 seconds
const unsigned long MQTT_UPDATES_EVERY_N_MILLIS = 1000;

DHTesp dht;

#ifdef ESP8266
	#define LED_BUILTIN_ON LOW
	#define LED_BUILTIN_OFF HIGH
#else // for ESP32
	#define LED_BUILTIN_ON HIGH
	#define LED_BUILTIN_OFF LOW
#endif

EspMQTTClient mqttClient(
	WIFI_SSID,
	WIFI_PASSWORD,
	MQTT_SERVER,
	MQTT_USERNAME,
	MQTT_PASSWORD,
	CLIENT_NAME,
	1883);

MQTTKalmanPublish mkTemp(mqttClient, BASE_TOPIC_STATUS "temp", MQTT_RETAINED, SEND_EVERY_TEMP, 0.2);
MQTTKalmanPublish mkHum(mqttClient, BASE_TOPIC_STATUS "hum", MQTT_RETAINED, SEND_EVERY_HUM, 2);
MQTTKalmanPublish mkRssi(mqttClient, BASE_TOPIC_STATUS "rssi", MQTT_RETAINED, SEND_EVERY_RSSI, 10);

void setup()
{
	pinMode(LED_BUILTIN, OUTPUT);
	Serial.begin(115200);
	Serial.println();

	Serial.print("MQTT Client Name: ");
	Serial.println(CLIENT_NAME);
	Serial.print("MQTT retained: ");
	Serial.println(MQTT_RETAINED ? "true" : "false");

	dht.setup(DHTPIN, IS_DHT11 ? DHTesp::DHT11 : DHTesp::DHT22);
	Serial.print("DHT Sensor type: ");
	if (dht.getModel() == DHTesp::DHT22)
		Serial.println("DHT22");
	else if (dht.getModel() == DHTesp::DHT11)
		Serial.println("DHT11");
	else
		Serial.println(dht.getModel());

	mqttClient.enableDebuggingMessages();
	mqttClient.enableHTTPWebUpdater();
	mqttClient.enableOTA();
	mqttClient.enableDrasticResetOnConnectionFailures();
	mqttClient.enableLastWillMessage(BASE_TOPIC "connected", "0", MQTT_RETAINED);
}

void onConnectionEstablished()
{
	mqttClient.publish(BASE_TOPIC "git-version", GIT_VERSION, MQTT_RETAINED);
}

void loop()
{
	mqttClient.loop();
	if (!mqttClient.isConnected()) {
		lastConnected = 0;
	}
	digitalWrite(LED_BUILTIN, lastConnected == 2 ? LED_BUILTIN_OFF : LED_BUILTIN_ON);

	auto now = millis();

	static unsigned long nextMeasure = 0;
	if (now < nextMeasure)
	{
		auto remaining = nextMeasure - now;
		if (remaining > MQTT_UPDATES_EVERY_N_MILLIS)
		{
			delay(MQTT_UPDATES_EVERY_N_MILLIS);
			return;
		}
		else
		{
			delay(remaining);
		}
	}

	nextMeasure = ((millis() / MEASURE_EVERY_N_MILLIS) + 1) * MEASURE_EVERY_N_MILLIS;

	// Read temperature as Celsius (the default)
	auto t = dht.getTemperature();
	auto h = dht.getHumidity();

	auto readSuccessful = dht.getStatus() == DHTesp::ERROR_NONE;
	auto nextConnected = readSuccessful ? 2 : 1;
	if (nextConnected != lastConnected && mqttClient.isConnected())
	{
		if (mqttClient.publish(BASE_TOPIC "connected", String(nextConnected), MQTT_RETAINED))
		{
			Serial.printf("set /connected from %d to %d\n", lastConnected, nextConnected);
			lastConnected = nextConnected;
		}
	}

	if (readSuccessful)
	{
		auto avgT = mkTemp.addMeasurement(t);
#ifdef DEBUG_KALMAN
		client.publish(BASE_TOPIC_STATUS "orig/temp", String(t), MQTT_RETAINED);
		client.publish(BASE_TOPIC_STATUS "avg/temp", String(avgT), MQTT_RETAINED);
#endif
		Serial.printf("Temperature in Celsius: %5.1f Average: %6.2f\n", t, avgT);

		auto avgH = mkHum.addMeasurement(h);
#ifdef DEBUG_KALMAN
		client.publish(BASE_TOPIC_STATUS "orig/hum", String(h), MQTT_RETAINED);
		client.publish(BASE_TOPIC_STATUS "avg/hum", String(avgH), MQTT_RETAINED);
#endif
		Serial.printf("Humidity    in Percent: %5.1f Average: %6.2f\n", h, avgH);
	}
	else
	{
		Serial.print("Failed to read from sensor! ");
		Serial.println(dht.getStatusString());
	}

	if (mqttClient.isWifiConnected())
	{
		auto rssi = WiFi.RSSI();
		auto avgRssi = mkRssi.addMeasurement(rssi);
#ifdef DEBUG_KALMAN
		client.publish(BASE_TOPIC_STATUS "orig/rssi", String(rssi), MQTT_RETAINED);
		client.publish(BASE_TOPIC_STATUS "avg/rssi", String(avgRssi), MQTT_RETAINED);
#endif
		Serial.printf("RSSI        in     dBm: %3d   Average: %6.2f\n", rssi, avgRssi);
	}
}
