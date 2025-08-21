#include "decode.h"

void parse_cstr(const char* data, size_t index, size_t length, char* output) {
    for (size_t i = 0; i < length && data[index + i] != '\0'; i++) {
        output[i] = data[index + i];
    }
    output[length] = '\0'; // Null-terminate the output string, in case it's not already
}

// NOTE: the output string should be 3 * length
void parse_bytes_str(const char* data, size_t index, size_t length, char* output) {
    for (size_t i = 0; i < length; i++) {
        sprintf(output + i * 3, "%02X ", static_cast<uint8_t>(data[index + i]));
    }
    output[length * 3 - 1] = '\0'; // Null-terminate the output string
}

// Only used to maintain parity with Python implementation
uint8_t parse_byte(const char* data, size_t index) {
    return static_cast<uint8_t>(data[index]);
}

uint16_t parse_16bit_unsigned(const char* data, size_t index) {
    return (static_cast<uint8_t>(data[index + 1]) << 8) | (static_cast<uint8_t>(data[index]) << 0);
}

uint32_t parse_32bit_unsigned(const char* data, size_t index) {
    return (static_cast<uint8_t>(data[index + 3]) << 24) |
           (static_cast<uint8_t>(data[index + 2]) << 16) |
           (static_cast<uint8_t>(data[index + 1]) << 8) |
           (static_cast<uint8_t>(data[index]) << 0);
}

int32_t parse_32bit_signed(const char* data, size_t index) {
    uint32_t value = parse_32bit_unsigned(data, index);
    return (data[index + 3] & 0x80) ? value - 0x100000000 : value; // Adjust for sign
}

uint8_t crc8(const uint8_t* byteData, size_t length) {
    uint8_t crc = 0;
    for (size_t i = 0; i < length; i++) {
        crc += byteData[i];
        crc &= 0xff; // Ensure crc is within 0-255
    }
    return crc;
}