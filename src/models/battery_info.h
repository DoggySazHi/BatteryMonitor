#ifndef BATTERY_INFO_H
#define BATTERY_INFO_H

#include "decode.h"

struct BatteryInfo {
    char header[12];
    char record_type[3];
    uint8_t record_counter;
    char device_model[16];
    char hardware_version[8];
    char software_version[8];
    uint32_t uptime;
    uint32_t powerCycles;
    char deviceName[16];
    char devicePasscode[16];
    char firstStartupDate[8];
    char serialNumber[12];
    char passcode[5];
    char userData[16];
    char setupPasscode[16];

    static void parseBatteryInfo(const unsigned char* data, BatteryInfo& info) {
        parse_bytes_str(data, 0, 4, info.header);
        parse_bytes_str(data, 4, 1, info.record_type);
        info.record_counter = parse_byte(data, 5);
        parse_cstr(data, 6, 16, info.device_model);
        parse_cstr(data, 22, 8, info.hardware_version);
        parse_cstr(data, 30, 8, info.software_version);

        info.uptime = parse_32bit_unsigned(data, 38);
        info.powerCycles = parse_32bit_unsigned(data, 42);
        parse_cstr(data, 46, 16, info.deviceName);
        parse_cstr(data, 62, 16, info.devicePasscode);
        parse_cstr(data, 78, 8, info.firstStartupDate);
        parse_cstr(data, 86, 11, info.serialNumber);
        parse_cstr(data, 97, 5, info.passcode);
        parse_cstr(data, 102, 16, info.userData);
        parse_cstr(data, 118, 16, info.setupPasscode);
    }

    void print() const {
        Serial.printf(
            "BatteryInfo(header=%s, record_type=%s, record_counter=%d, device_model=%s, hardware_version=%s, software_version=%s, "
            "uptime=%u, powerCycles=%u, deviceName=%s, devicePasscode=%s, firstStartupDate=%s, serialNumber=%s, passcode=%s, userData=%s, setupPasscode=%s)\n",
            header,
            record_type,
            record_counter,
            device_model,
            hardware_version,
            software_version,
            uptime,
            powerCycles,
            deviceName,
            devicePasscode,
            firstStartupDate,
            serialNumber,
            passcode,
            userData,
            setupPasscode);
    }
};

#endif // BATTERY_INFO_H