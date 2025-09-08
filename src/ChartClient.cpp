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

    if (
        strlen(batteryInfo->serialNumber) == 0 || // varchar(12)
        strlen(batteryInfo->serialNumber) > 12 ||
        cellInfo->average_cell_voltage < 0.0f || cellInfo->average_cell_voltage > 5.000f || // decimal(5, 3)
        cellInfo->delta_cell_voltage < 0.0f || cellInfo->delta_cell_voltage > 5.000f || // decimal(5, 3)
        cellInfo->mosfet_temperature < -40.0f || cellInfo->mosfet_temperature > 99.999f || // decimal(5, 3)
        cellInfo->battery_voltage < 0.0f || cellInfo->battery_voltage > 99.999f || // decimal(5, 3)
        cellInfo->battery_power < 0.0f || cellInfo->battery_power > 99.999f || // decimal(5, 3)
        fabs(cellInfo->battery_current) > 99.999f || // decimal(5, 3) - can be negative
        cellInfo->battery_temperature_1 < -9999.9f || cellInfo->battery_temperature_1 > 9999.9f || // decimal(5, 1)
        cellInfo->battery_temperature_2 < -9999.9f || cellInfo->battery_temperature_2 > 9999.9f || // decimal(5, 1)
        cellInfo->alarm_bits > 0xFFFFFFFF || // bigint (unsigned 32-bit)
        cellInfo->percent_remaining > 100 || // tinyint (0-100)
        cellInfo->remaining_capacity < 0.0f || cellInfo->remaining_capacity > 9999999.999f || // decimal(10, 3)
        cellInfo->nominal_capacity < 0.0f || cellInfo->nominal_capacity > 9999999.999f || // decimal(10, 3)
        cellInfo->cycle_capacity < 0.0f || cellInfo->cycle_capacity > 9999999.999f || // decimal(10, 3)
        cellInfo->state_of_health > 100 || // tinyint (0-100)
        cellInfo->cycle_count > 0xFFFFFFFF // bigint (unsigned 32-bit)
    ) {
        Serial.println("Cannot send data: data is corrupted");
        return;
    }

    Serial.println("Sending data to server...");
    // JSON - sprintf force all floats to be two decimal places (since that's our actual precision)

    const char* jsonTemplate = R"({
        "serial_number": "%s",
        "cell_info": {
            "cell_voltages": [
                %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f,
                %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f
            ],
            "average_cell_voltage": %.2f,
            "delta_cell_voltage": %.2f,
            "cell_wire_resistances": [
                %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f,
                %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f
            ],
            "mosfet_temperature": %.2f,
            "battery_voltage": %.2f,
            "battery_power": %.2f,
            "battery_current": %.2f,
            "battery_temperature_1": %.2f,
            "battery_temperature_2": %.2f,
            "alarm_bits": %u,
            "percent_remaining": %u,
            "remaining_capacity": %.2f,
            "nominal_capacity": %.2f,
            "cycle_capacity": %.2f,
            "state_of_health": %u,
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
    Serial.printf("POST %s\n", SERVER_ENDPOINT "/jkbms/ingest");
    Serial.printf("Payload: %s\n", buffer);

    http.addHeader("Content-Type", "application/json");
    Serial.printf("Content-Length: %zu\n", len);

    delay(100); // Small delay to allow connection to stabilize
    int httpResponseCode = http.POST((uint8_t*) buffer, len);
    if (httpResponseCode > 0) {
        Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    } else {
        Serial.printf("HTTP POST failed: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
}

void ChartClient::sendTestData() {
    if (!isConnected) {
        Serial.println("Cannot send data: not connected to WiFi");
        return;
    }

    Serial.println("Sending test data to server...");

    const char* testData = R"({
        "serial_number": "40729492166",
        "cell_info": {
            "cell_voltages": [
                3.38,
                3.38,
                3.38,
                3.38,
                3.38,
                3.38,
                3.38,
                3.38,
                3.38,
                3.38,
                3.38,
                3.38,
                3.38,
                3.38,
                3.38,
                3.38
            ],
            "average_cell_voltage": 3.38,
            "delta_cell_voltage": 0.01,
            "cell_wire_resistances": [
                0.08,
                0.08,
                0.10,
                0.11,
                0.13,
                0.13,
                0.15,
                0.17,
                0.17,
                0.16,
                0.15,
                0.12,
                0.11,
                0.10,
                0.09,
                0.08
            ],
            "mosfet_temperature": 33.50,
            "battery_voltage": 54.10,
            "battery_power": 1.47,
            "battery_current": 18.20,
            "battery_temperature_1": 29.30,
            "battery_temperature_2": 29.10,
            "alarm_bits": 0,
            "percent_remaining": 95,
            "remaining_capacity": 1.85,
            "nominal_capacity": 310.00,
            "cycle_capacity": 43.40,
            "state_of_health": 100,
            "cycle_count": 13
        }
    })";

    http.begin(client, SERVER_ENDPOINT "/jkbms/ingest");
    Serial.printf("POST %s\n", SERVER_ENDPOINT "/jkbms/ingest");
    Serial.printf("Payload: %s\n", testData);

    http.addHeader("Content-Type", "application/json");
    Serial.printf("Content-Length: %zu\n", strlen(testData));

    delay(100); // Small delay to allow connection to stabilize
    int httpResponseCode = http.POST((uint8_t*) testData, strlen(testData));
    if (httpResponseCode > 0) {
        Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    } else {
        Serial.printf("HTTP POST failed: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
}
