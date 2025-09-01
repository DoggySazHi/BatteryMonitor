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

#ifdef ARDUINO_RASPBERRY_PI_PICO_W

#include <btstack.h>
#include <btstack_run_loop.h>

typedef enum {
    TC_OFF,
    TC_IDLE,
    TC_W4_SCAN_RESULT,
    TC_W4_CONNECT,
    TC_W4_SERVICE_RESULT,
    TC_W4_CHARACTERISTIC_RESULT,
    TC_W4_ENABLE_NOTIFICATIONS_COMPLETE,
    TC_W4_READY
} gc_state_t;

btstack_packet_callback_registration_t JKBMS::hci_event_callback_registration;
gc_state_t JKBMS::state = TC_OFF;
bd_addr_t JKBMS::server_addr;
bd_addr_type_t JKBMS::server_addr_type;
hci_con_handle_t JKBMS::connection_handle;
gatt_client_service_t JKBMS::server_service;
gatt_client_characteristic_t JKBMS::server_characteristic;
bool JKBMS::listener_registered;
gatt_client_notification_t JKBMS::notification_listener;

void JKBMS::init() {
    l2cap_init();

    gatt_client_init();

    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);

    hci_event_callback_registration.callback = &JKBMS::handle_hci_event;

    // Start the Bluetooth stack
    hci_power_control(HCI_POWER_ON);
}

void JKBMS::handle_hci_event(uint8_t packet_type, uint16_t channel, unsigned char *packet, uint16_t size){
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;

    UNUSED(channel);
    UNUSED(size);

    uint8_t event_type = hci_event_packet_get_type(packet);
    switch(event_type){
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                gap_local_bd_addr(local_addr);
                printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
                client_start();
            } else {
                state = TC_OFF;
            }
            break;
        case GAP_EVENT_ADVERTISING_REPORT:
            if (state != TC_W4_SCAN_RESULT) return;
            // check name in advertisement
            if (!advertisement_report_contains_service(ORG_BLUETOOTH_SERVICE_ENVIRONMENTAL_SENSING, packet)) return;
            // store address and type
            gap_event_advertising_report_get_address(packet, server_addr);
            server_addr_type = gap_event_advertising_report_get_address_type(packet);
            // stop scanning, and connect to the device
            state = TC_W4_CONNECT;
            gap_stop_scan();
            printf("Connecting to device with addr %s.\n", bd_addr_to_str(server_addr));
            gap_connect(server_addr, server_addr_type);
            break;
        case HCI_EVENT_LE_META:
            // wait for connection complete
            switch (hci_event_le_meta_get_subevent_code(packet)) {
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                    if (state != TC_W4_CONNECT) return;
                    connection_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                    // initialize gatt client context with handle, and add it to the list of active clients
                    // query primary services
                    DEBUG_LOG("Search for env sensing service.\n");
                    state = TC_W4_SERVICE_RESULT;
                    gatt_client_discover_primary_services_by_uuid16(handle_gatt_client_event, connection_handle, ORG_BLUETOOTH_SERVICE_ENVIRONMENTAL_SENSING);
                    break;
                default:
                    break;
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            // unregister listener
            connection_handle = HCI_CON_HANDLE_INVALID;
            if (listener_registered){
                listener_registered = false;
                gatt_client_stop_listening_for_characteristic_value_updates(&notification_listener);
            }
            printf("Disconnected %s\n", bd_addr_to_str(server_addr));
            if (state == TC_OFF) break;
            client_start();
            break;
        default:
            break;
    }
}

void JKBMS::monitor() {
    btstack_run_loop_execute();
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
