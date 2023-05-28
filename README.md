# DoomPay

DoomPay is designed to be a framework so you can trigger "something" when a Square payment is received, using an ESP32.

Polls a Square account for new donations/payments. Doesn't (currently) care about the details of a payment, just that a new payment was received.

Keeps track of the last payment ID in persistent storage in case of crash or reboot to avoid re-triggering.

Embedded webserver and SoftAP mode for setting new WiFi credentials on-the-fly. Could easily be expanded for a status page such as money collected or games/triggers played.

Doesn't currently "do" anything except as described above. Adapt to suit your needs.

PlatformIO project for ESP32. Could easily be adapted to Arduino IDE.
