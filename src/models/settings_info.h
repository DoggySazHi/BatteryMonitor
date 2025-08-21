#ifndef SETTINGS_INFO_H
#define SETTINGS_INFO_H

#include "decode.h"

struct SettingsInfo {
    char header[12];
    char record_type[3];
    uint8_t record_counter;

    SettingsInfo(const char* data) {
        parse_bytes_str(data, 0, 12, header);
        parse_bytes_str(data, 12, 3, record_type);
        record_counter = parse_byte(data, 15);
    }

    String toString() const {
        String result = "SettingsInfo(";
        result.reserve(128);
        result += "header=" + String(header) + ", ";
        result += "record_type=" + String(record_type) + ", ";
        result += "record_counter=" + String(record_counter);
        result += ")";
        return result;
    }
};

#endif // SETTINGS_INFO_H