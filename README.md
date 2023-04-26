# ESP32_Sensor
This repo includes the ESP32 sensor code and additional header files. Use the Arduino IDE to upload code to ESP32.
## How it works
The ESP32_Sensor.ino code uses expressif libraries to run BLE based WiFi provisioning. 
Once this setup is complete, sensor measurements are taken every minute and data points are uploaded to InfluxDB every 30 seconds to account for possible connectivity losses. 
SHT30 sensor data is collected using an Adafruit library.
## Setup
1. Open repo with Arudino IDE
2. Create InfluxDB Cloud 2.0 account
3. Copy URL, TOKEN, and ORG numbers and define in a file named secrets.h
4. Upload project to ESP32
