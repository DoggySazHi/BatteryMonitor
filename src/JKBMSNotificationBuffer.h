#ifndef JKBMS_NOTIFICATION_BUFFER_H
#define JKBMS_NOTIFICATION_BUFFER_H

#include "models/constants.h"
#include "models/battery_info.h"
#include "models/settings_info.h"
#include "models/cell_info.h"

#include <NimBLEDevice.h>

class JKBMSNotificationBuffer {
public:
    bool handleNotification(unsigned char* data, size_t length);
    void resetParsedData();
    
    BatteryInfo* getBatteryInfo();
    SettingsInfo* getSettingsInfo();
    CellInfo* getCellInfo();
private:
    unsigned char notificationData[2048];
    size_t notificationLength = 0;

    BatteryInfo batteryInfo;
    SettingsInfo settingsInfo;
    CellInfo cellInfo;

    bool batteryInfoValid = false;
    bool settingsInfoValid = false;
    bool cellInfoValid = false;

    int findSOR();
    bool recordIsComplete();
    void processRecord();
    void shiftBufferToStart(size_t newStart);
};

#endif // JKBMS_NOTIFICATION_BUFFER_H