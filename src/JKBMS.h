#ifndef JK_BMS_H
#define JK_BMS_H

#include "models/constants.h"
#include "models/battery_info.h"
#include "models/settings_info.h"
#include "models/cell_info.h"

#include <NimBLEDevice.h>

#define SCAN_TIME 5000
#define SCAN_INTERVAL 100
#define SCAN_WINDOW 100
#define CONNECT_TIME 5000

class JKBMS : public NimBLEScanCallbacks
{
public:
    JKBMS(const NimBLEAddress& mac);

    static void init();
    void connect();
    void disconnect();
    void getBLEData();

private:
    NimBLEAddress macAddress;
    NimBLEScan* bleScan;
    NimBLEClient* bleClient;
    char bluetoothBuffer[2048];
    void notificationCallback(NimBLECharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify);
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice);
    void onScanEnd(const NimBLEScanResults& results, int reason);
};


#endif // JK_BMS_H