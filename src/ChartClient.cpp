#include "ChartClient.h"

void ChartClient::init() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("Connecting to WiFi network: %s\n", WIFI_SSID);
}

void ChartClient::monitor() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!isConnected) {
            isConnected = true;
            Serial.println("WiFi connected");
            Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        }
    } else {
        if (isConnected) {
            isConnected = false;
            Serial.printf("WiFi disconnected\n");
        }
    }
}

void ChartClient::sendData(const JKBMSNotificationBuffer& data) {
    if (!isConnected) {
        Serial.println("Cannot send data: not connected to WiFi");
        return;
    }

    const BatteryInfo* batteryInfo = data.getBatteryInfo();
    const CellInfo* cellInfo = data.getCellInfo();

    if (!batteryInfo || !cellInfo) {
        Serial.println("Cannot send data: incomplete information");
        return;
    }

    // JSON - sprintf force all floats to be two decimal places (since that's our actual precision)

    const char* jsonTemplate = R"({
        "serial_number": "%s",
        "cell_info": {
            "cell_voltages": [
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f
            ],
            "average_cell_voltage": %.2f,
            "delta_cell_voltage": %.2f,
            "cell_wire_resistances": [
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f,
                %.2f
            ],
            "mosfet_temperature": %.2f,
            "battery_voltage": %.2f,
            "battery_power": %.2f,
            "battery_current": %.2f,
            "battery_temperature_1": %.2f,
            "battery_temperature_2": %.2f,
            "alarm_bits": %hu,
            "percent_remaining": %hhu,
            "remaining_capacity": %.2f,
            "nominal_capacity": %.2f,
            "cycle_capacity": %.2f,
            "state_of_health": %hhu,
            "cycle_count": %u
        }
    })";

    size_t len = sprintf(buffer, jsonTemplate,
        batteryInfo->serialNumber,
        cellInfo->cell_voltages[0],
        cellInfo->cell_voltages[1],
        cellInfo->cell_voltages[2],
        cellInfo->cell_voltages[3],
        cellInfo->cell_voltages[4],
        cellInfo->cell_voltages[5],
        cellInfo->cell_voltages[6],
        cellInfo->cell_voltages[7],
        cellInfo->cell_voltages[8],
        cellInfo->cell_voltages[9],
        cellInfo->cell_voltages[10],
        cellInfo->cell_voltages[11],
        cellInfo->cell_voltages[12],
        cellInfo->cell_voltages[13],
        cellInfo->cell_voltages[14],
        cellInfo->cell_voltages[15],
        cellInfo->average_cell_voltage,
        cellInfo->delta_cell_voltage,
        cellInfo->cell_wire_resistances[0],
        cellInfo->cell_wire_resistances[1],
        cellInfo->cell_wire_resistances[2],
        cellInfo->cell_wire_resistances[3],
        cellInfo->cell_wire_resistances[4],
        cellInfo->cell_wire_resistances[5],
        cellInfo->cell_wire_resistances[6],
        cellInfo->cell_wire_resistances[7],
        cellInfo->cell_wire_resistances[8],
        cellInfo->cell_wire_resistances[9],
        cellInfo->cell_wire_resistances[10],
        cellInfo->cell_wire_resistances[11],
        cellInfo->cell_wire_resistances[12],
        cellInfo->cell_wire_resistances[13],
        cellInfo->cell_wire_resistances[14],
        cellInfo->cell_wire_resistances[15],
        cellInfo->mosfet_temperature,
        cellInfo->battery_voltage,
        cellInfo->battery_power,
        cellInfo->battery_current,
        cellInfo->battery_temperature_1,
        cellInfo->battery_temperature_2,
        cellInfo->alarm_bits,
        cellInfo->percent_remaining,
        cellInfo->remaining_capacity,
        cellInfo->nominal_capacity,
        cellInfo->cycle_capacity,
        cellInfo->state_of_health,
        cellInfo->cycle_count
    );

    http.begin(client, SERVER_ENDPOINT "/jkbms/ingest");

    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST((uint8_t*) buffer, len);
    if (httpResponseCode > 0) {
        Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    } else {
        Serial.printf("HTTP POST failed: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
}
