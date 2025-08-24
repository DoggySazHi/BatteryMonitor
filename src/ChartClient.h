#ifndef CHART_CLIENT_H
#define CHART_CLIENT_H

#include "constants.h"
#include "JKBMSNotificationBuffer.h"

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

class ChartClient {
public:
    void init();
    void monitor();
    void sendData(const JKBMSNotificationBuffer& data);
private:
    bool isConnected = false;
    WiFiClient _client;
    HTTPClient _http;
};

#endif