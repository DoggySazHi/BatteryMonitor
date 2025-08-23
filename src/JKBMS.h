#ifndef JK_BMS_H
#define JK_BMS_H

#include "models/constants.h"
#include "models/battery_info.h"
#include "models/settings_info.h"
#include "models/cell_info.h"
#include "JKBMSNotificationBuffer.h"

#include <NimBLEDevice.h>

#define SCAN_TIME 5000
#define SCAN_INTERVAL 30
#define SCAN_WINDOW 30
#define CONNECT_TIME 10000

class JKBMS : public NimBLEScanCallbacks, public NimBLEClientCallbacks
{
public:
    JKBMS(const NimBLEAddress& mac);

    static void init();
    void connect();
    void disconnect();
    void monitor();
    
    BatteryInfo* getBatteryInfo();
    SettingsInfo* getSettingsInfo();
    CellInfo* getCellInfo();

private:
    NimBLEAddress macAddress;
    NimBLEScan* bleScan;
    
    const NimBLEAdvertisedDevice* bleDevice = nullptr;
    bool readyToConnect = false;
    unsigned long lastActivity = 0;
    NimBLEClient* bleClient = nullptr;

    NimBLERemoteService* bleService = nullptr;
    NimBLERemoteCharacteristic* bleCharacteristic = nullptr;

    JKBMSNotificationBuffer buffer;

    void notificationCallback(NimBLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify);
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override;
    void onScanEnd(const NimBLEScanResults& results, int reason) override;
    void onConnect(NimBLEClient* pClient) override;
    void onConnectFail(NimBLEClient* pClient, int reason) override;
    void onDisconnect(NimBLEClient* pClient, int reason) override;
    void connectToDevice();
};


#endif // JK_BMS_H