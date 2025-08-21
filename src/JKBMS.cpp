#include "jkbms.h"

void JKBMS::init()
{
    NimBLEDevice::init("JKBMS");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9, NimBLETxPowerType::All);
}

JKBMS::JKBMS(const NimBLEAddress& mac)
{
    macAddress = mac;

    bleScan = NimBLEDevice::getScan();
    bleScan->setScanCallbacks(this);
    bleScan->setInterval(SCAN_INTERVAL);
    bleScan->setWindow(SCAN_WINDOW);
    bleScan->setActiveScan(true);
}

void JKBMS::connect()
{
    // Scan for devices
    bleScan->start(SCAN_TIME);
    Serial.println("Scanning for devices...");
}

void JKBMS::onResult(const NimBLEAdvertisedDevice* advertisedDevice)
{
    // Handle the result of the scan
    Serial.printf("Found device: %s\n", advertisedDevice->getAddress().toString().c_str());
}

void JKBMS::onScanEnd(const NimBLEScanResults& results, int reason)
{
    // Handle the end of the scan
    Serial.println("Scan ended");
}

void JKBMS::notificationCallback(NimBLECharacteristic *characteristic, uint8_t *data, size_t length, bool isNotify)
{

}
