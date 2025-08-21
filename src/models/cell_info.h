#ifndef CELL_INFO_H
#define CELL_INFO_H

#include "constants.h"
#include "decode.h"

struct CellInfo {
    char header[12];
    char record_type[3];
    uint8_t record_counter;
    float cell_voltages[16];
    float average_cell_voltage;
    float delta_cell_voltage;
    float cell_wire_resistances[16];
    float mosfet_temperature;
    float battery_voltage;
    float battery_power;
    float battery_current;
    float battery_temperature_1;
    float battery_temperature_2;
    uint8_t percent_remaining;
    float remaining_capacity;
    float nominal_capacity;
    float cycle_capacity;
    uint8_t state_of_health;
    uint32_t cycle_count;

    CellInfo(const char* data) {
        parse_bytes_str(data, 0, 12, header);
        parse_bytes_str(data, 12, 3, record_type);
        record_counter = parse_byte(data, 15);

        for (int i = 0; i < 16; i++) {
            cell_voltages[i] = parse_16bit_unsigned(data, 16 + i * 2) / 1000.0f;
        }

        average_cell_voltage = parse_16bit_unsigned(data, 48) / 1000.0f;
        delta_cell_voltage = parse_16bit_unsigned(data, 50) / 1000.0f;

        for (int i = 0; i < 16; i++) {
            cell_wire_resistances[i] = parse_16bit_unsigned(data, 52 + i * 2) / 1000.0f;
        }

        mosfet_temperature = parse_16bit_unsigned(data, 84) / 10.0f;

        battery_voltage = parse_32bit_unsigned(data, 86) / 1000.0f;
        battery_power = parse_16bit_unsigned(data, 90) / 1000.0f;
        battery_current = parse_32bit_signed(data, 94) / 1000.0f;

        battery_temperature_1 = parse_16bit_unsigned(data, 98) / 10.0f;
        battery_temperature_2 = parse_16bit_unsigned(data, 100) / 10.0f;

        uint16_t alarm_bits = parse_16bit_unsigned(data, 102);
        // Decoding the alarms - look at BATTERY_ERRORS - not done here to save memory

        percent_remaining = parse_byte(data, 118);
    }

    String toString() const {
        String result = "CellInfo(";
        result.reserve(512);
        result += "header=" + String(header) + ", ";
        result += "record_type=" + String(record_type) + ", ";
        result += "record_counter=" + String(record_counter) + ", ";
        result += "cell_voltages=[";
        for (int i = 0; i < 16; i++) {
            result += String(cell_voltages[i]);
            if (i < 15) result += ", ";
        }
        result += "], ";
        result += "average_cell_voltage=" + String(average_cell_voltage) + ", ";
        result += "delta_cell_voltage=" + String(delta_cell_voltage) + ", ";
        result += "cell_wire_resistances=[";
        for (int i = 0; i < 16; i++) {
            result += String(cell_wire_resistances[i]);
            if (i < 15) result += ", ";
        }
        result += "], ";
        result += "mosfet_temperature=" + String(mosfet_temperature) + ", ";
        result += "battery_voltage=" + String(battery_voltage) + ", ";
        result += "battery_power=" + String(battery_power) + ", ";
        result += "battery_current=" + String(battery_current) + ", ";
        result += "battery_temperature_1=" + String(battery_temperature_1) + ", ";
        result += "battery_temperature_2=" + String(battery_temperature_2) + ", ";
        result += "percent_remaining=" + String(percent_remaining) + ", ";
        result += "remaining_capacity=" + String(remaining_capacity) + ", ";
        result += "nominal_capacity=" + String(nominal_capacity) + ", ";
        result += "cycle_capacity=" + String(cycle_capacity) + ", ";
        result += "state_of_health=" + String(state_of_health) + ", ";
        result += "cycle_count=" + String(cycle_count);
        result += ")";
        return result;
    }
};

#endif // CELL_INFO_H