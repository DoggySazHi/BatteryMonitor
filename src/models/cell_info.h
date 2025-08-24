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

    static void parseCellInfo(const unsigned char* data, CellInfo& info) {
        parse_bytes_str(data, 0, 4, info.header);
        parse_bytes_str(data, 4, 1, info.record_type);
        info.record_counter = parse_byte(data, 5);

        for (int i = 0; i < 16; i++) {
            info.cell_voltages[i] = parse_16bit_unsigned(data, 6 + i * 2) / 1000.0f;
        }

        info.average_cell_voltage = parse_16bit_unsigned(data, 74) / 1000.0f;
        info.delta_cell_voltage = parse_16bit_unsigned(data, 76) / 1000.0f;

        for (int i = 0; i < 16; i++) {
            info.cell_wire_resistances[i] = parse_16bit_unsigned(data, 80 + i * 2) / 1000.0f;
        }

        info.mosfet_temperature = parse_16bit_unsigned(data, 144) / 10.0f;

        info.battery_voltage = parse_32bit_unsigned(data, 150) / 1000.0f;
        info.battery_power = parse_16bit_unsigned(data, 154) / 1000.0f;
        info.battery_current = parse_32bit_signed(data, 158) / 1000.0f;

        info.battery_temperature_1 = parse_16bit_unsigned(data, 162) / 10.0f;
        info.battery_temperature_2 = parse_16bit_unsigned(data, 164) / 10.0f;

        uint16_t alarm_bits = parse_16bit_unsigned(data, 166);
        // Decoding the alarms - look at BATTERY_ERRORS - not done here to save memory

        info.percent_remaining = parse_byte(data, 173);
        
        info.remaining_capacity = parse_32bit_unsigned(data, 174) / 1000.0f;
        info.nominal_capacity = parse_32bit_unsigned(data, 178) / 1000.0f;
        info.cycle_count = parse_32bit_unsigned(data, 182);
        info.cycle_capacity = parse_32bit_unsigned(data, 186) / 1000.0f;
        info.state_of_health = parse_byte(data, 190);
    }

    void print() const {
        Serial.printf(
            "CellInfo(header=%s, record_type=%s, record_counter=%d, cell_voltages=[",
            header,
            record_type,
            record_counter);
        for (int i = 0; i < 16; i++) {
            Serial.printf("%f", cell_voltages[i]);
            if (i < 15) Serial.printf(", ");
        }
        Serial.printf("], average_cell_voltage=%f, delta_cell_voltage=%f, cell_wire_resistances=[",
            average_cell_voltage,
            delta_cell_voltage);
        for (int i = 0; i < 16; i++) {
            Serial.printf("%f", cell_wire_resistances[i]);
            if (i < 15) Serial.printf(", ");
        }
        Serial.printf("], mosfet_temperature=%f, battery_voltage=%f, battery_power=%f, battery_current=%f, battery_temperature_1=%f, battery_temperature_2=%f, percent_remaining=%d, remaining_capacity=%f, nominal_capacity=%f, cycle_capacity=%f, state_of_health=%d, cycle_count=%d)\n",
            mosfet_temperature,
            battery_voltage,
            battery_power,
            battery_current,
            battery_temperature_1,
            battery_temperature_2,
            percent_remaining,
            remaining_capacity,
            nominal_capacity,
            cycle_capacity,
            state_of_health,
            cycle_count);
    }
};

#endif // CELL_INFO_H