# ESP MQTT DHT

This is the script for my [mqtt-smarthome](https://github.com/mqtt-smarthome/mqtt-smarthome) based Temperature Sensors (DHT22).
It is running on a NodeMCU board (ESP8266).

The basic source code was inspired by this [repo](https://github.com/titusece/ESP8266-MQTT-Publish-DHT11)

## Installation

In order to build this the arduino tool was used.

### MQTT Server

As an mqtt server I use a Raspberry Pi with mosquitto.
It is in the debian (so also in the raspbian) repository and can easily be installed: `sudo apt install mosquitto`.

### Prepare Arduino

TODO

### Adapt for yourself

duplicate the file `config.example.h` in order to create the `config.h` file.
Use the values you need for your environment.

When your mqtt server uses user/password comment the defines in.
The rest will use them then.
