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

    // If the buffer contains a ping response, dump the entire thing.
    for (size_t i = 0; i <= notificationLength - sizeof(PING_RESPONSE); ++i) {
        if (memcmp(&notificationData[i], PING_RESPONSE, sizeof(PING_RESPONSE)) == 0) {
            notificationLength = 0; // Discard all data if ping response found
            return false;
        }
    }

    // Search for the start of a record in the buffer
    int startIndex = findSOR();
    if (startIndex != -1) {
        shiftBufferToStart(startIndex);
    } else {
        notificationLength = 0; // Discard all data if no start found
        return false;
    }

    // Check if the length of the record is valid
    if (notificationLength > 300) { // 300 or 320
        return true;
    }

    return true;
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
    if (notificationData[5] == BATTERY_INFO_RECORD_TYPE) {
        BatteryInfo::parseBatteryInfo(notificationData, batteryInfo);
        batteryInfoValid = true;
    } else if (notificationData[5] == SETTINGS_INFO_RECORD_TYPE) {
        SettingsInfo::parseSettingsInfo(notificationData, settingsInfo);
        settingsInfoValid = true;
    } else if (notificationData[5] == CELL_INFO_RECORD_TYPE) {
        CellInfo::parseCellInfo(notificationData, cellInfo);
        cellInfoValid = true;
    } else {
        Serial.println("Unknown record type: " + String((int)notificationData[5]));
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

    Serial.printf("Received message of %d bytes. Buffer length after append: %d\n", length, notificationLength);
    for (size_t i = 0; i < notificationLength; ++i) {
        Serial.printf("\\x%02X", notificationData[i]);
    }
    Serial.println();

    // Process complete records in the buffer
    if (recordIsComplete()) {
        processRecord();
        notificationData[0] = 0; // Destroy SOR
        int nextSOR = findSOR();
        if (nextSOR != -1) {
            shiftBufferToStart(nextSOR);
        } else {
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
