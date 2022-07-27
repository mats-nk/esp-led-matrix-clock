# ESP8266-LED-Matrix-Clock

https://www.youtube.com/watch?v=mea5-qX4O54

Are required:
- 4 or 6 MAX7219 incl 8x LED-Matrix8x8
- one ESP8266 or ESP32 Board

Define GPIOs, Brigthness and Timezones in the platformio.ini.

Wifi credentials were recently replaced with [WiFiManager](https://github.com/tzapu/WiFiManager) - no hardcoded passwords anymore. When run for the first time (or no known networks present), configure via captive portal, then settings are saved and ESP will connect automatically.

Added support for ESP32-C3, tested on [Lolin C3 mini](https://www.wemos.cc/en/latest/c3/c3_mini.html) and works perfectly fine.

Brief description:

During setup, the current time is obtained from an NTP via WLAN. If the clock is in operation, this process is repeated daily 

### esp8266

![MatrixClock](/additional%20info/ESP8266_LED_Matrix_Clock.gif)

### esp32

![MatrixClock](/additional%20info/ESP32_MatrixClock.jpg)
![Schematic](/additional%20info/ESP32_MatrixClock_schematic.JPG)
