#ifndef JK_BMS_H
#define JK_BMS_H

#include "models/constants.h"
#include "models/battery_info.h"
#include "models/settings_info.h"
#include "models/cell_info.h"
#include "JKBMSNotificationBuffer.h"

#include <string>

#ifdef ESP32
#include <NimBLEDevice.h>

class JKBMS : public NimBLEScanCallbacks, public NimBLEClientCallbacks
{
public:
    JKBMS(const std::string& mac);

    static void init();
    void connect();
    void disconnect();
    void monitor();
    
    const BatteryInfo* getBatteryInfo() const;
    const SettingsInfo* getSettingsInfo() const;
    const CellInfo* getCellInfo() const;
    const JKBMSNotificationBuffer& getNotificationBuffer() const;
    void resetParsedData();

    bool isRunning() const;

private:
    NimBLEAddress macAddress;
    NimBLEScan* bleScan;

    bool runFlag = false; // Atomic flag to indicate if the BMS is running
    const NimBLEAdvertisedDevice* bleDevice = nullptr;
    bool readyToConnect = false;
    bool readyToExchange = false;
    unsigned long lastActivity = 0;
    NimBLEClient* bleClient = nullptr;

    NimBLERemoteService* bleService = nullptr;
    NimBLERemoteCharacteristic* bleCharacteristic = nullptr;

    JKBMSNotificationBuffer buffer;

    void notificationCallback(NimBLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify);
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override;
    void onScanEnd(const NimBLEScanResults& results, int reason) override;
    void onConnect(NimBLEClient* pClient) override;
    void onPostConnect();
    void onConnectFail(NimBLEClient* pClient, int reason) override;
    void onDisconnect(NimBLEClient* pClient, int reason) override;
    void connectToDevice();
};

#endif

#ifdef ARDUINO_RASPBERRY_PI_PICO_W
#include <btstack.h>

class JKBMS {
public:
    JKBMS(const std::string& mac);

    static void init();
    void connect();
    void disconnect();
    void monitor();
    
    const BatteryInfo* getBatteryInfo() const;
    const SettingsInfo* getSettingsInfo() const;
    const CellInfo* getCellInfo() const;
    const JKBMSNotificationBuffer& getNotificationBuffer() const;
    void resetParsedData();

    bool isRunning() const;
private:
    bool runFlag = false; // Atomic flag to indicate if the BMS is running
    bool readyToConnect = false;
    bool readyToExchange = false;
    unsigned long lastActivity = 0;
    JKBMSNotificationBuffer buffer;
};

#endif


#endif // JK_BMS_H