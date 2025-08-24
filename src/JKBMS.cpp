#include "JKBMS.h"

void JKBMS::init() {
    NimBLEDevice::init("JKBMS");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9, NimBLETxPowerType::All);
    NimBLEDevice::setMTU(512);
}

JKBMS::JKBMS(const NimBLEAddress& mac) {
    macAddress = mac;

    bleScan = NimBLEDevice::getScan();
    bleScan->setInterval(SCAN_INTERVAL);
    bleScan->setWindow(SCAN_WINDOW);
    bleScan->setActiveScan(true);
}

void JKBMS::connect() {
    lastActivity = millis();
    runFlag = true;
    buffer.resetParsedData();

    // Scan for devices
    bleScan->setScanCallbacks(this);
    bleScan->start(SCAN_TIME);
    Serial.println("Scanning for devices...");
}

void JKBMS::disconnect() {
    if (bleClient && bleClient->isConnected()) {
        bleClient->disconnect(BLE_ERR_SUCCESS);
        BLEDevice::deleteClient(bleClient);
        Serial.println("Disconnected from device");
    }

    Serial.printf("Destroying BLE client %p\n", bleClient);
    bleClient = nullptr;
    bleDevice = nullptr;
    bleService = nullptr;
    bleCharacteristic = nullptr;
    runFlag = false;
}

void JKBMS::onResult(const NimBLEAdvertisedDevice* advertisedDevice) {
    // Handle the result of the scan
    Serial.printf("Found device: %s\n", advertisedDevice->getAddress().toString().c_str());
    if (advertisedDevice->getAddress() == macAddress) {
        Serial.println("Found target device, connecting...");
        bleDevice = advertisedDevice;
        bleScan->stop();
        readyToConnect = true;
    }
}

void JKBMS::onScanEnd(const NimBLEScanResults& results, int reason)
{
    // Handle the end of the scan
    Serial.println("Scan ended");
    runFlag = false;
}

void JKBMS::onConnect(NimBLEClient* pClient) {
    Serial.printf("Connected to: %s\n", pClient->getPeerAddress().toString().c_str());
    lastActivity = millis();
    readyToExchange = true;
}

void JKBMS::onPostConnect() {
#ifdef JKBMS_DEBUG
    Serial.printf("Current MTU: %d\n", bleClient->getMTU());
#endif

    lastActivity = millis();
    // NOTE: ble_att_clt_tx_mtu in ble_att_clt.c needs to be modified to not set BLE_HS_EALREADY
    Serial.printf("Locating attributes...\n");
    bleClient->discoverAttributes();

#ifdef JKBMS_DEBUG
    // Print all available services
    for (const auto& service : bleClient->getServices(false)) {
        Serial.printf("Found service: %s\n", service->getUUID().toString().c_str());
    }
#endif

    // Discover services and characteristics
    bleService = bleClient->getService(NimBLEUUID("FFE0"));
    if (bleService) {
        Serial.printf("Found service: %s\n", bleService->getUUID().toString().c_str());

        // Discover characteristics
#ifdef JKBMS_DEBUG
        for (const auto& characteristic : bleService->getCharacteristics(true)) {
            Serial.printf("Found characteristic: %s\n", characteristic->getUUID().toString().c_str());
        }
#endif

        bleCharacteristic = bleService->getCharacteristic("FFE1");
    }

    if (bleCharacteristic) {
#ifdef JKBMS_DEBUG
        Serial.printf("Found characteristic: %s\n", bleCharacteristic->getUUID().toString().c_str());
#endif
        Serial.println("Subscribing to notifications...");
        bleCharacteristic->subscribe(true, [this](NimBLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify) {
            notificationCallback(characteristic, data, length, isNotify);
        }, false);
        Serial.println("Requesting battery info...");
        bleCharacteristic->writeValue(GET_BATTERY_INFO);
    }
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

#ifdef JKBMS_DEBUG
        Serial.println("Notification processed successfully");
        Serial.printf("- Battery info: %s\n", buffer.getBatteryInfo() ? "Received" : "N/A");
        Serial.printf("- Settings info: %s\n", buffer.getSettingsInfo() ? "Received" : "N/A");
        Serial.printf("- Cell info: %s\n", buffer.getCellInfo() ? "Received" : "N/A");
#endif

        if (buffer.getBatteryInfo() && !buffer.getCellInfo() && !buffer.getSettingsInfo()) {
            // If we have battery data, we should send a request for cell data
            buffer.getBatteryInfo()->print();
            Serial.println("Requesting settings info...");
            bleCharacteristic->writeValue(GET_SETTINGS_INFO);
        } else if (buffer.getSettingsInfo() && !buffer.getCellInfo()) {
            // If we have settings data, we should send a request for cell data
            buffer.getSettingsInfo()->print();
            Serial.println("Requesting cell info...");
        } else if (buffer.getCellInfo()) {
            // If we have cell data, we're done - disconnect
            buffer.getCellInfo()->print();
            Serial.println("Received cell info, disconnecting...");
            disconnect();
        }
    }
}

void JKBMS::monitor() {
    unsigned long currentTime = millis();

    if (readyToConnect && bleDevice) {
        readyToConnect = false;
        connectToDevice();
    }

    if (readyToExchange && currentTime - lastActivity > EXCHANGE_TIME && lastActivity - currentTime > INTERRUPT_MAX_DESYNC) {
        // Start exchanging data
        readyToExchange = false;
        onPostConnect();
    }

    // Due to interrupts, lastActivity may be greater than currentTime - need to handle this scenario due to underflow
    if (currentTime - lastActivity > ACTIVITY_TIMEOUT && lastActivity - currentTime > INTERRUPT_MAX_DESYNC && bleDevice) {
        // No activity - disconnect
        Serial.printf("No activity - disconnecting (%d ms)\n", currentTime - lastActivity);
        disconnect();
    }
}

void JKBMS::connectToDevice() {
    if (NimBLEDevice::getCreatedClientCount()) {
        // See if we can free up a client

        NimBLEClient* tempBleClient = NimBLEDevice::getClientByPeerAddress(bleDevice->getAddress());
        if (tempBleClient) {
            // Delete the existing client
            Serial.printf("Deleted existing client\n");
            NimBLEDevice::deleteClient(tempBleClient);
        } else {
            // Delete a stale client
            tempBleClient = NimBLEDevice::getDisconnectedClient();
            Serial.printf("Deleted stale client\n");
            NimBLEDevice::deleteClient(tempBleClient);
        }
    }

    if (NimBLEDevice::getCreatedClientCount() >= NIMBLE_MAX_CONNECTIONS) {
        Serial.println("No available clients, cannot allocate connection");
        disconnect();
        return;
    }

    Serial.printf("Creating new client for device: %s\n", bleDevice->getAddress().toString().c_str());
    lastActivity = millis();

    bleClient = NimBLEDevice::createClient(bleDevice->getAddress());
    bleClient->setSelfDelete(true, true);
    bleClient->setClientCallbacks(this, false);
    bleClient->setConnectionParams(32, 160, 0, 500);
    bleClient->setConnectTimeout(CONNECT_TIME);

    if (!bleClient->connect(true, true, true)) {
        Serial.println("Failed to connect to device");
        disconnect();
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

void JKBMS::resetParsedData() {
    buffer.resetParsedData();
}

bool JKBMS::isRunning() const {
    return runFlag;
}
