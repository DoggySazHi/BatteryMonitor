#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <LittleFS.h>

class Config {
public:
    static Config& getInstance();
    static void save();

    uint8_t lastBMSChecked = 0;
private:
    static Config instance;
    static bool initialized;

    static void init();
};

#endif // CONFIG_H