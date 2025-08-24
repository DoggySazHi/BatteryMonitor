#include "constants.h"

// Bluetooth
#include "JKBMS.h"
JKBMS bmsDevices[] = {
    JKBMS(NimBLEAddress("C8:47:80:20:2E:B3", 0)),
    JKBMS(NimBLEAddress("98:DA:10:07:AC:56", 0)),
    JKBMS(NimBLEAddress("C8:47:80:21:72:6D", 0)),
    JKBMS(NimBLEAddress("C8:47:80:3A:22:F3", 0))
};

#define NUM_BMS_DEVICES (sizeof(bmsDevices) / sizeof(bmsDevices[0]))

// WiFi
#ifdef USE_WIFI
#include "ChartClient.h"
ChartClient chartClient;
#endif

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
#endif
#ifdef USE_LEDS
void turnOffLEDs();
#endif

void checkJKBMS();

void setup() {
    Serial.begin(115200);

#ifdef USE_LEDS
    // Turn off LEDs
    turnOffLEDs();
#endif

#ifdef USE_TOUCH
    // Enable the backlight
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    // Initialise the display
    tft.init();
    tft.setRotation(1); //This is the display in landscape
    
    // Initialise the touchscreen
    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(mySpi);
    ts.setRotation(1);
    
    // Clear the screen before writing to it
    tft.fillScreen(TFT_BLACK);
#endif

#ifdef USE_WIFI
    chartClient.init();
#endif

    JKBMS::init();
}

void loop() {
#ifdef USE_TOUCH
    checkTouchScreen();
#endif

#ifdef USE_WIFI
    chartClient.monitor();
#endif

    checkJKBMS();
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
#endif

void checkJKBMS() {
    boolean allHaveCellInfo = true;

    for (int i = 0; i < NUM_BMS_DEVICES; i++) {
        if (bmsDevices[i].getCellInfo()) {
            continue; // Skip this device if it already has cell info
        }

        if (!bmsDevices[i].isRunning()) {
            bmsDevices[i].connect();
            Serial.printf("Connecting to BMS device %d...\n", i + 1);
        }

        bmsDevices[i].monitor();

        if (!bmsDevices[i].getCellInfo()) {
            allHaveCellInfo = false;
        } else {
            Serial.printf("BMS device %d cell info:\n", i + 1);
            bmsDevices[i].getCellInfo()->print();

#ifdef USE_WIFI
            chartClient.sendData(bmsDevices[i].getNotificationBuffer());
#endif
        }

        // Only one device can be connected at a time
        break;
    }

    // If all devices have cell info, reset them all
    if (allHaveCellInfo) {
        for (int i = 0; i < NUM_BMS_DEVICES; i++) {
            bmsDevices[i].resetParsedData();
        }
    }
}

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
