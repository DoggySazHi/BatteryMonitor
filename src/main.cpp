#include "constants.h"

// Watchdog
#ifdef ESP32
#include <esp_task_wdt.h>
#endif
unsigned long startupTime = 0;

// Bluetooth
#include "JKBMS.h"
JKBMS bmsDevices[] = {
    JKBMS("C8:47:80:20:2E:B3"),
    JKBMS("98:DA:10:07:AC:56"),
    JKBMS("C8:47:80:21:72:6D"),
    JKBMS("C8:47:80:3A:22:F3")
};

#define NUM_BMS_DEVICES (sizeof(bmsDevices) / sizeof(bmsDevices[0]))

// WiFi
#ifdef USE_WIFI
#include "ChartClient.h"
ChartClient chartClient;
#endif

// Config
#include "Config.h"

#ifdef USE_TOUCH
// Touchscreen and display
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
SPIClass mySpi = SPIClass(VSPI);
TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
#endif

// Forward declarations
#ifdef USE_TOUCH
void checkTouchScreen();
void renderJKBMS();
#endif
#ifdef USE_LEDS
void turnOffLEDs();
#endif

void feedWatchdog();
void resetDevice();
void delaySafe(unsigned long ms);
void checkJKBMS();

void setup() {
    Serial.begin(115200);

#ifdef USE_LEDS
    // Turn off LEDs
    turnOffLEDs();
#endif

#ifdef ESP32
    esp_task_wdt_deinit();
    esp_err_t watchdogError = esp_task_wdt_init(WATCHDOG_TIMEOUT, true);
    Serial.println("Last Reset : " + String(esp_err_to_name(watchdogError)));
    esp_task_wdt_add(NULL);  //add current thread to WDT watch
#endif
#ifdef ARDUINO_ARCH_RP2040
    delay(1000);
    rp2040.wdt_begin(WATCHDOG_TIMEOUT * 1000);
    RP2040::resetReason_t resetReason = rp2040.getResetReason();

    switch (resetReason) {
        case RP2040::resetReason_t::WDT_RESET:
            Serial.println("Reset Reason: Watchdog Timer");
            break;
        case RP2040::resetReason_t::PWRON_RESET:
            Serial.println("Reset Reason: Power On");
            break;
        case RP2040::resetReason_t::SOFT_RESET:
            Serial.println("Reset Reason: Software");
            break;
        default:
            Serial.println("Reset Reason: Unknown");
            break;
    }
#endif
    startupTime = millis();

#ifdef USE_TOUCH
    // Initialise the display
    tft.init();
    tft.setRotation(1); //This is the display in landscape

    // Enable the backlight
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    
    // Initialise the touchscreen
    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(mySpi);
    ts.setRotation(1);
#endif

#ifdef USE_WIFI
    chartClient.init();
#endif

    JKBMS::init();
}

void loop() {
#ifdef USE_TOUCH
    checkTouchScreen();
    renderJKBMS();
#endif

    // Feed the watchdog
    feedWatchdog();

#ifdef USE_WIFI
    chartClient.monitor();
#endif // USE_WIFI

#if defined(ARDUINO_ARCH_RP2040) and defined(USE_WIFI)

    if (!WiFi.isConnected()) {
        Serial.println("WiFi not connected, skipping BMS check");
        delaySafe(1000);
    } else {
        checkJKBMS();
    }

#else
    // Check as normal.
    checkJKBMS();
#endif // ARDUINO_ARCH_RP2040

    if (millis() - startupTime > EXECUTION_TIMEOUT) {
        Serial.println("Execution timeout reached");

#ifdef ARDUINO_ARCH_RP2040
        Config& config = Config::getInstance();
        config.lastBMSChecked += 1;
        Config::save();
#endif

        resetDevice();
    }
}

void resetDevice() {
#ifdef ESP32
    ESP.restart();
#endif
#ifdef ARDUINO_ARCH_RP2040
    rp2040.reboot();
#endif
}

void feedWatchdog() {
#ifdef ESP32
    esp_task_wdt_reset();
#endif
#ifdef ARDUINO_ARCH_RP2040
    rp2040.wdt_reset();
#endif
}

void delaySafe(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        feedWatchdog();
        delay(10);
    }
}

#ifdef USE_TOUCH
unsigned long lastTouchTime = 0;
void checkTouchScreen() {
    unsigned long currentMillis = millis();

    if (ts.touched() && (currentMillis - lastTouchTime > DEBOUNCE_TIME)) {
        lastTouchTime = currentMillis;
        TS_Point p = ts.getPoint();
        Serial.printf("Touch detected at (%d, %d)\n", p.x, p.y);
    }
}

unsigned long lastRenderTime = 0;
char statusBuffer[128];
void renderJKBMS() {
    unsigned long currentMillis = millis();

    if (currentMillis - lastRenderTime < 1000 / 10) {
        return;
    }

    lastRenderTime = currentMillis;

    // Clear the screen before writing to it
    tft.fillScreen(TFT_BLACK);

    for (int i = 0; i < NUM_BMS_DEVICES; i++) {
        sprintf(statusBuffer, "BMS %d: %s", i + 1, (bmsDevices[i].isRunning() ? (bmsDevices[i].getCellInfo() ? "Data Received" : "Loading") : "Disconnected"));

        if (bmsDevices[i].getCellInfo()) {
            tft.setTextColor(TFT_GREEN);
        } else if (bmsDevices[i].isRunning()) {
            tft.setTextColor(TFT_YELLOW);
        } else {
            tft.setTextColor(TFT_RED);
        }

        tft.drawString(statusBuffer, 0, i * 20);
    }
}
#endif

#ifdef ESP32
size_t bmsIndex = 0;
void checkJKBMS() {
    for (int i = 0; i < NUM_BMS_DEVICES; i++) {
        if (bmsDevices[i].getCellInfo()) {
            continue; // Skip this device if it already has cell info
        }

        if (!bmsDevices[i].isRunning()) {
            if (bmsIndex < i + 1) {
                bmsIndex = i + 1;
            } else {
                continue; // Skip this device if it failed to connect
            }

            Serial.printf("Connecting to BMS device %d...\n", i + 1);
            delaySafe(5000); // Delay to allow previous disconnect to settle
            bmsDevices[i].connect();
        }

        bmsDevices[i].monitor();

        // Only one device can be connected at a time
        break;
    }

    // If all devices have cell info, reset them all
    if (bmsDevices[NUM_BMS_DEVICES - 1].getCellInfo() || bmsIndex >= NUM_BMS_DEVICES && !bmsDevices[NUM_BMS_DEVICES - 1].isRunning()) {
        for (int i = 0; i < NUM_BMS_DEVICES; i++) {
            if (!bmsDevices[i].getCellInfo()) {
                continue;
            }

            Serial.printf("BMS device %d cell info:\n", i + 1);
            bmsDevices[i].getCellInfo()->print();

#ifdef USE_WIFI
            chartClient.sendData(bmsDevices[i].getNotificationBuffer());
#endif

            bmsDevices[i].resetParsedData();
        }
        
        bmsIndex = 0;

        Serial.println("All devices processed, resetting...");
        delaySafe(5000);
    }
}
#endif

#ifdef ARDUINO_ARCH_RP2040

void checkJKBMS() {
    Config& config = Config::getInstance();
    uint8_t lastBMSChecked = config.lastBMSChecked % NUM_BMS_DEVICES;

    // Only probe one BMS device at a time then reset the module - workaround for RP2040 BTStack stability issues
    if (!bmsDevices[lastBMSChecked].isRunning()) {
        Serial.printf("Connecting to BMS device %d...\n", lastBMSChecked + 1);
        bmsDevices[lastBMSChecked].connect();
    }

    bmsDevices[lastBMSChecked].monitor();

    if (bmsDevices[lastBMSChecked].getCellInfo()) {
        Serial.printf("BMS device %d cell info:\n", lastBMSChecked + 1);
        bmsDevices[lastBMSChecked].getCellInfo()->print();

#ifdef USE_WIFI
        chartClient.sendData(bmsDevices[lastBMSChecked].getNotificationBuffer());
#endif
        
        config.lastBMSChecked = lastBMSChecked + 1;
        Config::save();

        Serial.printf("Device %d processed, resetting...\n", lastBMSChecked + 1);
        delaySafe(5000);
        resetDevice();
    }
}

#endif

#ifdef USE_LEDS
void turnOffLEDs() {
    pinMode(LED_BLUE, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    // LEDs at HIGH are off, not LOW - don't ask why
    digitalWrite(LED_BLUE, HIGH);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, HIGH);
}
#endif
