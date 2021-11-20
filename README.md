# ESP MQTT DHT

This is the script for my [mqtt-smarthome](https://github.com/mqtt-smarthome/mqtt-smarthome) based Temperature Sensors (DHT22).
It is running on a NodeMCU board (ESP8266).

The basic source code was inspired by this [repo](https://github.com/titusece/ESP8266-MQTT-Publish-DHT11)

## Installation

In order to build this the Arduino tool was used.

### MQTT Server

As an MQTT server I use a Raspberry Pi with `mosquitto`.
It is in the Debian repository (which includes Raspberry Pi OS) and can easily be installed: `sudo apt install mosquitto`.

Make sure to allow anonymous (and enable the default listener on 1883 if you also want web socket support)

```conf
listener 1883

allow_anonymous true
```

### Prepare Arduino

Set the following under File → Preferences → Settings → Additional Board Manager Urls (hit the rightmost button to open the Window):

```plaintext
https://arduino.esp8266.com/stable/package_esp8266com_index.json
https://dl.espressif.com/dl/package_esp32_index.json
```

Next Steps… TODO
