# DoomPay

DoomPay is designed to be a framework so you can trigger "something" when a Square payment is received, using an ESP32.

Polls a Square account for new donations/payments. Doesn't (currently) care about the details of a payment, just that a new payment was received.

Keeps track of the last payment ID in persistent storage in case of crash or reboot to avoid re-triggering.

Embedded webserver and SoftAP mode for setting new WiFi credentials on-the-fly. Could easily be expanded for a status page such as money collected or games/triggers played.

Uses HTTPS, but does zero verification of remote server certificates. Don't use this for anything serious.

Doesn't currently "do" anything except as described above. Adapt to suit your needs.

Create your own my_config.h file with: 

```
#define DEFAULT_SSID "SSID for SoftAP Mode" // WiFi SSID for SoftAP mode, like "DoomPay"
#define DEFAULT_PASS "Default SoftAP Mode Password" // Currently required to be changed the first time you use it. 
#define SQUARE_AUTH "Bearer AAAAAAA....AAAAA" // Your secret key for Square API. Keep this a secret.
```

PlatformIO project for ESP32. Can easily be adapted to Arduino IDE - just grab main.cpp and rename it to whatever.ino.

External libraries (already defined in PlatformIO.ini):

* [bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson) 
* [bblanchon/StreamUtils](https://github.com/bblanchon/StreamUtils) 
* [prampec/IotWebConf](https://github.com/prampec/IotWebConf) 
