# IoT-bike-thing

An IoT bike ride recording device for the minimalist.

# Building The Bike Thing
## Hardware
* SparkFun ESP32 Thing
* Sparkfun 9DoF Stick (LSM9DS1)
* SparkFun GPS Receiver (GP-20U7)	
* SparkFun Level Shifting microSD Reader
* SanDisk microSDHC Card 8GB	
* Lithium Ion Battery - 2Ah

## Wiring

![Suggested use of various lengths of wire](https://github.com/um-iot/IoT-bike-thing/blob/master/images/wiring.png)

## Software

### Environment setup
Setup the ESP environment as per http://esp-idf.readthedocs.io/en/latest/get-started/linux-setup.html

Setup Arduino as a component of esp-idf
https://github.com/espressif/arduino-esp32/blob/master/docs/esp-idf_component.md


## Flash

`cd /IoT-Bike-Thing/`

`make menuconfig`
* Set port to usb port (/dev/ttyUSB0)
* Set baud rate to 9600

## Recording

Press record button - wait for red record light to become stable
Press record again to stop recording

## Viewing your data

Connect to WiFi
Maps should load your data automatically - on tonights docket

## Connecting to Wifi

Press wifi on button
Wait for it to connect
Load web page in browser - it is a private network site at: 192.168.xxx.xxx

// TODO: currently ssid and password must be configured at compile (super unsafe!!!!)  

