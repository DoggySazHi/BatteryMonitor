#ifndef JK_BMS_H
#define JK_BMS_H

#include "models/constants.h"
#include "models/battery_info.h"
#include "models/settings_info.h"
#include "models/cell_info.h"
#include "JKBMSNotificationBuffer.h"

#include <string>

#ifdef ESP32
#include <NimBLEDevice.h>

class JKBMS : public NimBLEScanCallbacks, public NimBLEClientCallbacks
{
public:
    JKBMS(const std::string& mac);

    static void init();
    void connect();
    void disconnect();
    void monitor();
    
    const BatteryInfo* getBatteryInfo() const;
    const SettingsInfo* getSettingsInfo() const;
    const CellInfo* getCellInfo() const;
    const JKBMSNotificationBuffer& getNotificationBuffer() const;
    void resetParsedData();

    bool isRunning() const;

private:
    NimBLEAddress macAddress;
    NimBLEScan* bleScan;

    bool runFlag = false; // Atomic flag to indicate if the BMS is running
    const NimBLEAdvertisedDevice* bleDevice = nullptr;
    bool readyToConnect = false;
    bool readyToExchange = false;
    unsigned long lastActivity = 0;
    NimBLEClient* bleClient = nullptr;

    NimBLERemoteService* bleService = nullptr;
    NimBLERemoteCharacteristic* bleCharacteristic = nullptr;

    JKBMSNotificationBuffer buffer;

    void notificationCallback(NimBLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify);
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override;
    void onScanEnd(const NimBLEScanResults& results, int reason) override;
    void onConnect(NimBLEClient* pClient) override;
    void onPostConnect();
    void onConnectFail(NimBLEClient* pClient, int reason) override;
    void onDisconnect(NimBLEClient* pClient, int reason) override;
    void connectToDevice();
};

#endif

#ifdef ARDUINO_RASPBERRY_PI_PICO_W
#include <btstack.h>

class JKBMS {
public:
    JKBMS(const std::string& mac);

    static void init();
    void connect();
    void disconnect();
    void monitor();
    
    const BatteryInfo* getBatteryInfo() const;
    const SettingsInfo* getSettingsInfo() const;
    const CellInfo* getCellInfo() const;
    const JKBMSNotificationBuffer& getNotificationBuffer() const;
    void resetParsedData();

    bool isRunning() const;
private:
    bd_addr_t macAddress;
    bd_addr_type_t macAddressType; // bd_addr_type_t
    hci_con_handle_t connectionHandle = HCI_CON_HANDLE_INVALID;
    bool serviceFound = false;
    gatt_client_service_t remoteService;
    gatt_client_characteristic_t remoteCharacteristic;
    bool listenerRegistered = false;
    gatt_client_notification_t notificationListener;

    bool runFlag = false; // Atomic flag to indicate if the BMS is running
    unsigned long lastActivity = 0;
    JKBMSNotificationBuffer buffer;

    // For some reason, btstack does not allow const data in the write function, so we need this workaround
    unsigned char sendBuffer[sizeof(GET_BATTERY_INFO)];

    static void static_handle_hci_event(uint8_t packet_type, uint16_t channel, unsigned char *packet, uint16_t size);
    static void static_handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
    // The static functions above are used to bind to the C-style callback system of btstack.
    static btstack_packet_callback_registration_t hci_event_callback_registration;
    static JKBMS* activeInstance;

    void handle_hci_event(uint8_t packet_type, uint16_t channel, unsigned char *packet, uint16_t size);
    void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
};

#endif


#endif // JK_BMS_H