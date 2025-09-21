#include "Config.h"

Config Config::instance;
bool Config::initialized = false;

Config& Config::getInstance() {
    if (!initialized) {
        init();
    }

    return instance;
}

void Config::save() {
    File configFile = LittleFS.open("/config.dat", "w");
    if (configFile) {
        configFile.write((const uint8_t *) &instance, sizeof(instance));
        configFile.close();
    } else {
        Serial.println("Failed to open config file for writing");
    }
}

void Config::init() {
    if (!LittleFS.begin()) {
        Serial.println("An error has occurred while mounting LittleFS");
    } else {
        Serial.println("LittleFS mounted successfully");
    }

    bool readOK = false;

    // Check if config.dat exists
    if (LittleFS.exists("/config.dat")) {
        File configFile = LittleFS.open("/config.dat", "r");
        if (configFile) {
            // Read the configuration data
            configFile.readBytes((char *) &instance, sizeof(instance));
            configFile.close();
            readOK = true;
        }
    }

    if (!readOK) {
        // If reading failed, initialize default values
        save();
    }

    initialized = true;
}