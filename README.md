# ESP8266-LED-Matrix-Clock

https://www.youtube.com/watch?v=mea5-qX4O54

Are required:
- 4 or 6 MAX7219 incl 8x LED-Matrix8x8
- one ESP8266 or ESP32 Board

define GPIOs, Credentials, Brigthness and Timezones in the platformio.ini.

Brief description:
During setup, the current time is obtained from an NTP via WLAN. If the clock is in operation, this process is repeated daily 

### esp8266

![MatrixClock](/additional%20info/ESP8266_LED_Matrix_Clock.gif)

### esp32

![MatrixClock](/additional%20info/ESP32_MatrixClock.jpg)
![Schematic](/additional%20info/ESP32_MatrixClock_schematic.JPG)
