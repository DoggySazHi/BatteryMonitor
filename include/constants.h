#ifndef CONSTANTS_H
#define CONSTANTS_H

// WiFi credentials
#if __has_include("private.h")
    #include "private.h"
#endif

#ifndef WIFI_SSID
    #define WIFI_SSID "default_ssid"
#endif

#ifndef WIFI_PASSWORD
    #define WIFI_PASSWORD "default_password"
#endif

#ifndef SERVER_ENDPOINT
    #define SERVER_ENDPOINT "http://your.server.endpoint"
#endif

// Watchdog
#define WATCHDOG_TIMEOUT 15

// Touchscreen
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
#define DEBOUNCE_TIME 50

// LED
#define LED_BLUE 17
#define LED_RED 4
#define LED_GREEN 16

#endif