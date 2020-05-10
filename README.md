ESPToy
======

ESPToy is an open-source development board for the ESP8266 WiFi module

- v1 has an on-board avr (atmega644) microcontroller which communicates with an external ESP8266-01 module through serial.

- v1.2x has built-in ESP12 WiFi chip, color LED, button, LiPo battery jack and charging chip, microUSB connector and CH340G USB-serial. Its demo program is in arduino/v1.2x folder. Uploading new program requires bootloading procedure (i.e. hold the on-board button while plugging in USB to enter bootloading mode).

- v2.0 has added auto reset circuit (eliminating bootloading procedure), changed LED to WS2812 (Neopixel) type, added a mini buzzer, light sensor and solder jumper to enable/disable the sensor. Its demo program is in arduino/v2.0 folder. V2.0 is fully compatible with LOLIN Wemos D1 Mini.
