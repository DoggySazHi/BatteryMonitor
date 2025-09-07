#ifndef JKBMS_NOTIFICATION_BUFFER_H
#define JKBMS_NOTIFICATION_BUFFER_H

#include "models/constants.h"
#include "models/battery_info.h"
#include "models/settings_info.h"
#include "models/cell_info.h"

class JKBMSNotificationBuffer {
public:
    bool handleNotification(const unsigned char* data, size_t length);
    void resetParsedData();
    
    const BatteryInfo* getBatteryInfo() const;
    const SettingsInfo* getSettingsInfo() const;
    const CellInfo* getCellInfo() const;
private:
    unsigned char notificationData[NOTIFICATION_BUFFER_SIZE];
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