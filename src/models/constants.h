#ifndef MODELS_CONSTANTS_H
#define MODELS_CONSTANTS_H

#include <Arduino.h>

#define SETTINGS_INFO_RECORD_TYPE 1
#define CELL_INFO_RECORD_TYPE 2
#define BATTERY_INFO_RECORD_TYPE 3

static const unsigned char GET_BATTERY_INFO[] PROGMEM = "\xaa\x55\x90\xeb\x97\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x11";
static const unsigned char GET_CELL_INFO[] PROGMEM = "\xaa\x55\x90\xeb\x96\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10";

static const unsigned char START_OF_RECORD[] = { 0xaa, 0x55, 0x90, 0xeb };
static const unsigned char PING_RESPONSE[] = { 0x55, 0xaa, 0xeb, 0x90 };
static const unsigned char KEEP_ALIVE[] = "AT\r\n";

static const unsigned char BATTERY_ERROR_0[] PROGMEM = "Abnormal wire resistance";
static const unsigned char BATTERY_ERROR_1[] PROGMEM = "MOSFET overtemperature";
static const unsigned char BATTERY_ERROR_2[] PROGMEM = "Cell count not equal to settings";
static const unsigned char BATTERY_ERROR_3[] PROGMEM = "Current sensor abnormal";
static const unsigned char BATTERY_ERROR_4[] PROGMEM = "Cell overvoltage";
static const unsigned char BATTERY_ERROR_5[] PROGMEM = "Full battery overvoltage";
static const unsigned char BATTERY_ERROR_6[] PROGMEM = "Charge overcurrent";
static const unsigned char BATTERY_ERROR_7[] PROGMEM = "Charge short circuit";
static const unsigned char BATTERY_ERROR_8[] PROGMEM = "Overtemperature";
static const unsigned char BATTERY_ERROR_9[] PROGMEM = "Undertemperature";
static const unsigned char BATTERY_ERROR_10[] PROGMEM = "Coprocessor error";
static const unsigned char BATTERY_ERROR_11[] PROGMEM = "Cell undervoltage";
static const unsigned char BATTERY_ERROR_12[] PROGMEM = "Full battery undervoltage";
static const unsigned char BATTERY_ERROR_13[] PROGMEM = "Discharge overcurrent";
static const unsigned char BATTERY_ERROR_14[] PROGMEM = "Discharge short circuit";
static const unsigned char BATTERY_ERROR_15[] PROGMEM = "Discharge overtemperature";

static const unsigned char* BATTERY_ERRORS[] PROGMEM = {
    BATTERY_ERROR_0,
    BATTERY_ERROR_1,
    BATTERY_ERROR_2,
    BATTERY_ERROR_3,
    BATTERY_ERROR_4,
    BATTERY_ERROR_5,
    BATTERY_ERROR_6,
    BATTERY_ERROR_7,
    BATTERY_ERROR_8,
    BATTERY_ERROR_9,
    BATTERY_ERROR_10,
    BATTERY_ERROR_11,
    BATTERY_ERROR_12,
    BATTERY_ERROR_13,
    BATTERY_ERROR_14,
    BATTERY_ERROR_15
};

#endif // MODELS_CONSTANTS_H