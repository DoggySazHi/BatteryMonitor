#ifndef DECODE_H
#define DECODE_H

#include <Arduino.h>

void parse_cstr(const unsigned char* data, size_t index, size_t length, char* output);

// NOTE: the output string should be 3 * length
void parse_bytes_str(const unsigned char* data, size_t index, size_t length, char* output);

// Only used to maintain parity with Python implementation
uint8_t parse_byte(const unsigned char* data, size_t index);

uint16_t parse_16bit_unsigned(const unsigned char* data, size_t index);

uint32_t parse_32bit_unsigned(const unsigned char* data, size_t index);

int32_t parse_32bit_signed(const unsigned char* data, size_t index);

uint8_t crc8(const uint8_t* byteData, size_t length);

#endif // DECODE_H