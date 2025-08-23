#include "jkbms.h"

void JKBMS::init() {
    NimBLEDevice::init("JKBMS");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9, NimBLETxPowerType::All);
    NimBLEDevice::setMTU(512);
}

JKBMS::JKBMS(const NimBLEAddress& mac) {
    macAddress = mac;

    bleScan = NimBLEDevice::getScan();
    bleScan->setScanCallbacks(this);
    bleScan->setInterval(SCAN_INTERVAL);
    bleScan->setWindow(SCAN_WINDOW);
    bleScan->setActiveScan(true);
}

void JKBMS::connect() {
    lastActivity = millis();
    buffer.resetParsedData();

    // Scan for devices
    bleScan->start(SCAN_TIME);
    Serial.println("Scanning for devices...");
}

void JKBMS::disconnect() {
    if (bleClient && bleClient->isConnected()) {
        bleClient->disconnect(BLE_ERR_SUCCESS);
        Serial.println("Disconnected from device");
    }

    bleClient = nullptr;
    bleDevice = nullptr;
    bleService = nullptr;
    bleCharacteristic = nullptr;
}

void JKBMS::onResult(const NimBLEAdvertisedDevice* advertisedDevice) {
    // Handle the result of the scan
    Serial.printf("Found device: %s\n", advertisedDevice->getAddress().toString().c_str());
    if (advertisedDevice->getAddress() == macAddress) {
        Serial.println("Found target device, connecting...");
        bleScan->stop();
        bleDevice = advertisedDevice;
        readyToConnect = true;
    }
}

void JKBMS::onScanEnd(const NimBLEScanResults& results, int reason)
{
    // Handle the end of the scan
    Serial.println("Scan ended");
}

void JKBMS::onConnect(NimBLEClient* pClient) {
    Serial.printf("Connected to: %s\n", pClient->getPeerAddress().toString().c_str());
    lastActivity = millis();

    // Discover services and characteristics
    bleService = pClient->getService("FFE0");
    if (bleService) {
        bleCharacteristic = bleService->getCharacteristic("FFE1");
    }

    bleCharacteristic->subscribe(true, [this](NimBLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify) {
        notificationCallback(characteristic, data, length, isNotify);
    }, false);
    bleCharacteristic->writeValue(GET_BATTERY_INFO);
}

void JKBMS::onConnectFail(NimBLEClient* pClient, int reason) {
    Serial.printf("Failed to connect to: %s, reason = %d\n", pClient->getPeerAddress().toString().c_str(), reason);
    bleClient = nullptr;
    disconnect();
}

void JKBMS::onDisconnect(NimBLEClient* pClient, int reason) {
    Serial.printf("%s Disconnected, reason =%d\n", pClient->getPeerAddress().toString().c_str(), reason);
    bleClient = nullptr;
    disconnect();
}

void JKBMS::notificationCallback(NimBLERemoteCharacteristic *characteristic, uint8_t *data, size_t length, bool isNotify) {
    if (buffer.handleNotification(data, length)) {
        lastActivity = millis();

        if (buffer.getBatteryInfo()) {
            // If we have battery data, we should send a request for cell data
            bleCharacteristic->writeValue(GET_CELL_INFO);
        } else if (buffer.getCellInfo()) {
            // If we have cell data, we're done - disconnect
            disconnect();
        }
    }
}

void JKBMS::monitor() {
    unsigned long currentTime = millis();

    if (!bleClient) {
        // Not running, don't care.
        return;
    }

    if (currentTime - lastActivity > CONNECT_TIME) {
        // No activity - disconnect
        Serial.println("No activity - disconnecting");
        disconnect();
    }

    if (readyToConnect && bleDevice) {
        readyToConnect = false;
        connectToDevice();
    }
}

void JKBMS::connectToDevice() {
    if (NimBLEDevice::getCreatedClientCount()) {
        // See if we can free up a client

        NimBLEClient* tempBleClient = NimBLEDevice::getClientByPeerAddress(bleDevice->getAddress());
        if (tempBleClient) {
            // Delete the existing client
            Serial.printf("Deleted existing client: %s\n", tempBleClient->getPeerAddress().toString().c_str());
            NimBLEDevice::deleteClient(tempBleClient);
        } else {
            // Delete a stale client
            tempBleClient = NimBLEDevice::getDisconnectedClient();
            Serial.printf("Deleted stale client: %s\n", tempBleClient->getPeerAddress().toString().c_str());
            NimBLEDevice::deleteClient(tempBleClient);
        }
    }

    if (NimBLEDevice::getCreatedClientCount() >= NIMBLE_MAX_CONNECTIONS) {
        Serial.println("No available clients, cannot allocate connection");
        bleDevice = nullptr;
        return;
    }

    Serial.printf("Creating new client for device: %s\n", bleDevice->getAddress().toString().c_str());

    bleClient = NimBLEDevice::createClient(bleDevice->getAddress());
    bleClient->setSelfDelete(true, true);
    bleClient->setClientCallbacks(this, false);
    bleClient->setConnectionParams(32, 160, 0, 500);
    bleClient->setConnectTimeout(CONNECT_TIME);

    if (!bleClient->connect(true, true, true)) {
        Serial.println("Failed to connect to device");
        bleClient = nullptr;
        bleDevice = nullptr;
        return;
    }
}

BatteryInfo* JKBMS::getBatteryInfo() {
    return buffer.getBatteryInfo();
}

SettingsInfo* JKBMS::getSettingsInfo() {
    return buffer.getSettingsInfo();
}

CellInfo* JKBMS::getCellInfo() {
    return buffer.getCellInfo();
}
