#include "ChartClient.h"

void ChartClient::init() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("Connecting to WiFi network: %s\n", WIFI_SSID);
}

void ChartClient::monitor() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!isConnected) {
            isConnected = true;
            Serial.println("WiFi connected");
            Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());

            _http.begin(_client, SERVER_ENDPOINT);
        }
    } else {
        if (isConnected) {
            isConnected = false;
            Serial.printf("WiFi disconnected\n");

            _http.end();
        }
    }
}

void ChartClient::sendData(const JKBMSNotificationBuffer& data) {
    if (!isConnected) {
        Serial.println("Cannot send data: not connected to WiFi");
        return;
    }
}
