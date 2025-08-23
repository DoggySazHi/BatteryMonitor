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

    String toString() const {
        String result = "BatteryInfo(";
        result.reserve(512); // Reserve enough space for the string
        result += "header=" + String(header) + ", ";
        result += "record_type=" + String(record_type) + ", ";
        result += "record_counter=" + String(record_counter) + ", ";
        result += "device_model=" + String(device_model) + ", ";
        result += "hardware_version=" + String(hardware_version) + ", ";
        result += "software_version=" + String(software_version) + ", ";
        result += "uptime=" + String(uptime) + ", ";
        result += "powerCycles=" + String(powerCycles) + ", ";
        result += "deviceName=" + String(deviceName) + ", ";
        result += "devicePasscode=" + String(devicePasscode) + ", ";
        result += "firstStartupDate=" + String(firstStartupDate) + ", ";
        result += "serialNumber=" + String(serialNumber) + ", ";
        result += "passcode=" + String(passcode) + ", ";
        result += "userData=" + String(userData) + ", ";
        result += "setupPasscode=" + String(setupPasscode);
        result += ")";
        return result;
    }
};

#endif // BATTERY_INFO_H