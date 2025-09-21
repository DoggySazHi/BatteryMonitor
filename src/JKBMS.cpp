#include "JKBMS.h"

#ifdef ESP32
void JKBMS::init() {
    NimBLEDevice::init("JKBMS");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9, NimBLETxPowerType::All);
    NimBLEDevice::setMTU(512);
}

JKBMS::JKBMS(const std::string& mac) {
    macAddress = NimBLEAddress(mac, 0);

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
        lastActivity = millis();
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

    if (!isRunning() || !bleClient || !bleClient->isConnected()) {
        return; // We should not be here
    }

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
            Serial.println("Requesting settings info...");
            bleCharacteristic->writeValue(GET_SETTINGS_INFO);
        } else if (buffer.getSettingsInfo() && !buffer.getCellInfo()) {
            // If we have settings data, we should send a request for cell data
            Serial.println("Requesting cell info...");
        } else if (buffer.getCellInfo()) {
            // If we have cell data, we're done - disconnect
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
#endif

#ifdef ARDUINO_ARCH_RP2040

#include <btstack.h>
#include <btstack_run_loop.h>

btstack_packet_callback_registration_t JKBMS::hci_event_callback_registration;
JKBMS* JKBMS::activeInstance = nullptr;

JKBMS::JKBMS(const std::string& mac) {
    // Convert MAC address string to bd_addr_t
    
    for (int i = 0; i < 6; i++) {
        std::string byteString = mac.substr(i * 3, 2);
        macAddress[i] = (uint8_t) strtol(byteString.c_str(), nullptr, 16);
    }

    macAddressType = BD_ADDR_TYPE_LE_PUBLIC;
}

void JKBMS::init() {
    l2cap_init();

    att_server_init(nullptr, nullptr, nullptr);

    gatt_client_init();

    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);

    hci_event_callback_registration.callback = &JKBMS::static_handle_hci_event;
    hci_add_event_handler(&hci_event_callback_registration);

    // Start the Bluetooth stack
    hci_power_control(HCI_POWER_ON);

    Serial.println("Bluetooth initialized");
}

void JKBMS::connect() {
    // Start scanning
    lastActivity = millis();
    runFlag = true;
    buffer.resetParsedData();

    // Bind event handlers
    activeInstance = this;

    // Scan for devices
    gap_set_scan_params(1, 48, 48, 0);
    gap_start_scan();
    Serial.println("Scanning for devices...");
}

void JKBMS::disconnect() {
    if (connectionHandle != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(connectionHandle);
        Serial.println("Disconnected from device");
    }

    // Stop scanning if still active
    gap_stop_scan();
    Serial.println("Stopped scanning");

    activeInstance = nullptr;
    connectionHandle = HCI_CON_HANDLE_INVALID;
    serviceFound = false;
    listenerRegistered = false;
    batteryInfoSent = false;

    runFlag = false;

    Serial.println("Cleanup complete");
}

void JKBMS::static_handle_hci_event(uint8_t packet_type, uint16_t channel, unsigned char *packet, uint16_t size) {
    if (activeInstance) {
        activeInstance->handle_hci_event(packet_type, channel, packet, size);
    }
}

void JKBMS::static_handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (activeInstance) {
        activeInstance->handle_gatt_client_event(packet_type, channel, packet, size);
    }
}

void JKBMS::handle_hci_event(uint8_t packet_type, uint16_t channel, unsigned char *packet, uint16_t size) {
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;

    bd_addr_t targetMacAddress;
    bd_addr_type_t targetMacAddressType;
    uint8_t event_type = hci_event_packet_get_type(packet);

    switch(event_type) {
        case GAP_EVENT_ADVERTISING_REPORT:
            gap_event_advertising_report_get_address(packet, targetMacAddress);
            targetMacAddressType = (bd_addr_type_t) gap_event_advertising_report_get_address_type(packet);
            // Serial.printf("Found device: %s\n", bd_addr_to_str(targetMacAddress));

            if (memcmp(targetMacAddress, macAddress, 6) == 0) {
                Serial.printf("Found target device: %s\n", bd_addr_to_str(targetMacAddress));

                gap_stop_scan();
                Serial.printf("Connecting to device with addr %s.\n", bd_addr_to_str(targetMacAddress));
                gap_connect(targetMacAddress, targetMacAddressType);
                lastActivity = millis();
            }

            break;
        case HCI_EVENT_LE_META:
            // wait for connection complete
            switch (hci_event_le_meta_get_subevent_code(packet)) {
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                    connectionHandle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                    Serial.printf("Connected, handle %u\n", connectionHandle);
                    
                    gatt_client_discover_primary_services_by_uuid16(static_handle_gatt_client_event, connectionHandle, 0xFFE0);
                    lastActivity = millis();
                    break;
                default:
                    break;
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            // Handle already lost connection
            connectionHandle = HCI_CON_HANDLE_INVALID;

            printf("Disconnected %s\n", bd_addr_to_str(macAddress));
            disconnect();
            break;
        default:
            break;
    }
}

void JKBMS::handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);

    uint8_t att_status;
    switch(hci_event_packet_get_type(packet)){
        case GATT_EVENT_SERVICE_QUERY_RESULT:
            Serial.println("Storing service");
            gatt_event_service_query_result_get_service(packet, &remoteService);
            break;
        case GATT_EVENT_QUERY_COMPLETE:
            att_status = gatt_event_query_complete_get_att_status(packet);
            
            if (att_status != ATT_ERROR_SUCCESS){
                printf("GATT_QUERY_RESULT, ATT Error 0x%02x.\n", att_status);
                disconnect();
                break;  
            } 

            if (!serviceFound) {
                serviceFound = true;
                Serial.println("Searching for battery characteristic");
                gatt_client_discover_characteristics_for_service_by_uuid16(static_handle_gatt_client_event, connectionHandle, &remoteService, 0xFFE1);
                lastActivity = millis();
            } else if (!listenerRegistered) {
                // register handler for notifications
                listenerRegistered = true;
                gatt_client_listen_for_characteristic_value_updates(&notificationListener, static_handle_gatt_client_event, connectionHandle, &remoteCharacteristic);
                // enable notifications
                Serial.println("Subscribing to notifications...");
                gatt_client_write_client_characteristic_configuration(static_handle_gatt_client_event, connectionHandle, &remoteCharacteristic, GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
            } else {
                if (!batteryInfoSent) {
                    batteryInfoSent = true;
                    Serial.printf("Notifications enabled, ATT status 0x%02x\n", gatt_event_query_complete_get_att_status(packet));
                    delay(100); // Short delay to ensure notifications are enabled before sending data
                    Serial.println("Requesting battery info...");
                    memcpy((void*) sendBuffer, (const void*) GET_BATTERY_INFO, sizeof(GET_BATTERY_INFO));
                    gatt_client_write_value_of_characteristic(static_handle_gatt_client_event, connectionHandle, remoteCharacteristic.value_handle, sizeof(GET_BATTERY_INFO), sendBuffer);
                }
            }

            break;
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
            Serial.println("Locating characteristic...");
            gatt_event_characteristic_query_result_get_characteristic(packet, &remoteCharacteristic);
            lastActivity = millis();
            break;
        case GATT_EVENT_NOTIFICATION: {
            uint16_t length = gatt_event_notification_get_value_length(packet);
            const uint8_t *data = gatt_event_notification_get_value(packet);

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
                    Serial.println("Requesting settings info...");
                    memcpy((void*) sendBuffer, (const void*) GET_SETTINGS_INFO, sizeof(GET_SETTINGS_INFO));
                    gatt_client_write_value_of_characteristic(static_handle_gatt_client_event, connectionHandle, remoteCharacteristic.value_handle, sizeof(GET_SETTINGS_INFO), sendBuffer);
                } else if (buffer.getSettingsInfo() && !buffer.getCellInfo()) {
                    // If we have settings data, we should send a request for cell data
                    Serial.println("Requesting cell info...");
                } else if (buffer.getCellInfo()) {
                    // If we have cell data, we're done - disconnect
                    Serial.println("Received cell info, disconnecting...");
                    disconnect();
                }
            }

            break;
        }
        default:
            printf("Unknown packet type 0x%02x\n", hci_event_packet_get_type(packet));
            break;
    }
}

void JKBMS::monitor() {
    unsigned long currentTime = millis();

    // BTstack is running in the background - no need to call a poll function

    // Due to interrupts, lastActivity may be greater than currentTime - need to handle this scenario due to underflow
    if (isRunning() && currentTime - lastActivity > ACTIVITY_TIMEOUT && lastActivity - currentTime > INTERRUPT_MAX_DESYNC) {
        // No activity - disconnect
        Serial.printf("No activity - disconnecting (%d ms)\n", currentTime - lastActivity);
        disconnect();
    }
}

#endif

const BatteryInfo* JKBMS::getBatteryInfo() const {
    return buffer.getBatteryInfo();
}

const SettingsInfo* JKBMS::getSettingsInfo() const {
    return buffer.getSettingsInfo();
}

const CellInfo* JKBMS::getCellInfo() const {
    return buffer.getCellInfo();
}

const JKBMSNotificationBuffer& JKBMS::getNotificationBuffer() const {
    return buffer;
}

void JKBMS::resetParsedData() {
    buffer.resetParsedData();
}

bool JKBMS::isRunning() const {
    return runFlag;
}
