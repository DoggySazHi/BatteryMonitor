#ifndef SETTINGS_INFO_H
#define SETTINGS_INFO_H

#include "decode.h"

struct SettingsInfo {
    char header[12];
    char record_type[3];
    uint8_t record_counter;

    static void parseSettingsInfo(const unsigned char* data, SettingsInfo& info) {
        parse_bytes_str(data, 0, 12, info.header);
        parse_bytes_str(data, 12, 3, info.record_type);
        info.record_counter = parse_byte(data, 15);
    }

    void print() const {
        Serial.printf("SettingsInfo(header=%s, record_type=%s, record_counter=%d)\n", header, record_type, record_counter);
    }
};

#endif // SETTINGS_INFO_H