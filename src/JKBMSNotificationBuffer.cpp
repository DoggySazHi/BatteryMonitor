#include "JKBMSNotificationBuffer.h"

int JKBMSNotificationBuffer::findSOR() {
    for (size_t i = 0; i <= notificationLength - sizeof(START_OF_RECORD);++i) {
        if (memcmp(&notificationData[i], START_OF_RECORD, sizeof(START_OF_RECORD)) == 0) {
            return i;
        }
    }

    return -1;
}

bool JKBMSNotificationBuffer::recordIsComplete() {
    // Check if the buffer contains the start of a record
    if (notificationLength < sizeof(START_OF_RECORD)) {
        return false;
    }

    // Search for the start of a record in the buffer
    int startIndex = findSOR();
    if (startIndex != -1) {
#ifdef JKBMS_DEBUG
        Serial.printf("Start of record found at index: %d\n", startIndex);
#endif
        shiftBufferToStart(startIndex);
    } else {
#ifdef JKBMS_DEBUG
        Serial.println("No start of record found, discarding buffer.");
#endif
        notificationLength = 0; // Discard all data if no start found
        return false;
    }

    // Check if the length of the record is valid
    if (notificationLength > 300) { // 300 or 320
        return true;
    }

    return false;
}

void JKBMSNotificationBuffer::shiftBufferToStart(size_t newStart) {
    if (newStart < notificationLength) {
        memmove(notificationData, &notificationData[newStart], notificationLength - newStart);
        notificationLength -= newStart;
    } else {
        notificationLength = 0;
    }
}

void JKBMSNotificationBuffer::processRecord() {
    if (notificationData[4] == BATTERY_INFO_RECORD_TYPE) {
        BatteryInfo::parseBatteryInfo(notificationData, batteryInfo);
#ifdef JKBMS_DEBUG
        Serial.println("Parsed battery info");
#endif
        batteryInfoValid = true;
    } else if (notificationData[4] == SETTINGS_INFO_RECORD_TYPE) {
        SettingsInfo::parseSettingsInfo(notificationData, settingsInfo);
#ifdef JKBMS_DEBUG
        Serial.println("Parsed settings info");
#endif
        settingsInfoValid = true;
    } else if (notificationData[4] == CELL_INFO_RECORD_TYPE) {
        CellInfo::parseCellInfo(notificationData, cellInfo);
#ifdef JKBMS_DEBUG
        Serial.println("Parsed cell info");
#endif
        cellInfoValid = true;
    } else {
        Serial.printf("Unknown record type: %02X\n", notificationData[4]);
    }
}

bool JKBMSNotificationBuffer::handleNotification(unsigned char* data, size_t length) {
    if (length + notificationLength > sizeof(notificationData)) {
        // If incoming data exceeds buffer size, reset buffer
        notificationLength = 0;
    }

    // Append new data to the buffer
    for (size_t i = 0; i < length; ++i) {
        if (memcmp(&data[i], KEEP_ALIVE, sizeof(KEEP_ALIVE)) == 0) {
            // If ping response found, skip those bytes
            i += sizeof(KEEP_ALIVE) - 1;
            continue;
        }

        notificationData[notificationLength++] = data[i];
    }

#ifdef JKBMS_DEBUG
    Serial.printf("Received message of %d bytes. Buffer length after append: %d\n", length, notificationLength);
    for (size_t i = 0; i < notificationLength; ++i) {
        Serial.printf("\\x%02X", notificationData[i]);
    }
    Serial.println();
#endif

    // Process complete records in the buffer
    if (recordIsComplete()) {
        processRecord();
        notificationData[0] = 0; // Destroy SOR
        int nextSOR = findSOR();
        if (nextSOR != -1) {
#ifdef JKBMS_DEBUG
            Serial.printf("Next start of record found at index: %d\n", nextSOR);
#endif
            shiftBufferToStart(nextSOR);
        } else {
#ifdef JKBMS_DEBUG
            Serial.println("No next start of record found, discarding buffer.");
#endif
            notificationLength = 0; // Discard all data if no start found
        }

        return true;
    }

    return false;
}

void JKBMSNotificationBuffer::resetParsedData() {
    batteryInfoValid = false;
    settingsInfoValid = false;
    cellInfoValid = false;
}

BatteryInfo* JKBMSNotificationBuffer::getBatteryInfo() {
    return batteryInfoValid ? &batteryInfo : nullptr;
}

SettingsInfo* JKBMSNotificationBuffer::getSettingsInfo() {
    return settingsInfoValid ? &settingsInfo : nullptr;
}

CellInfo* JKBMSNotificationBuffer::getCellInfo() {
    return cellInfoValid ? &cellInfo : nullptr;
}
