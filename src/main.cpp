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

// PNG decoding
#ifdef USE_PNG
#include "reimu.h"
#include <PNGdec.h>
PNG png; // PNG decoder instance
#endif

// Touchscreen and display
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
SPIClass mySpi = SPIClass(VSPI);
TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

// Forward declarations
void checkTouchScreen();
void checkJKBMS();
void turnOffLEDs();
#ifdef USE_PNG
void drawReimu();
int pngCallback(PNGDRAW* pDraw);
#endif

void setup()
{
    Serial.begin(115200);

    // Turn off LEDs
    turnOffLEDs();

    // Initialise the display
    tft.init();
    tft.setRotation(1); //This is the display in landscape
    
    // Initialise the touchscreen
    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(mySpi);
    ts.setRotation(1);
    
    // Clear the screen before writing to it
    tft.fillScreen(TFT_BLACK);
#ifdef USE_PNG
    drawReimu();
#endif
    JKBMS::init();
}

void loop()
{
    checkTouchScreen();
    checkJKBMS();
}

unsigned long lastTouchTime = 0;
void checkTouchScreen() {
    unsigned long currentMillis = millis();

    if (ts.touched() && (currentMillis - lastTouchTime > DEBOUNCE_TIME)) {
        lastTouchTime = currentMillis;
        TS_Point p = ts.getPoint();
        // Serial.printf("Touch detected at (%d, %d)\n", p.x, p.y);
    }
}

void checkJKBMS() {
    for (int i = 0; i < NUM_BMS_DEVICES; i++) {
        if (bmsDevices[i].getCellInfo()) {
            continue; // Skip this device if it already has cell info
        }

        if (!bmsDevices[i].isRunning()) {
            bmsDevices[i].connect();
            Serial.printf("Connecting to BMS device %d...\n", i + 1);
        }

        bmsDevices[i].monitor();

        // Only one device can be connected at a time
        break;
    }
}

void turnOffLEDs() {
    pinMode(LED_BLUE, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    // LEDs at HIGH are off, not LOW - don't ask why
    digitalWrite(LED_BLUE, HIGH);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, HIGH);
}

#ifdef USE_PNG
void drawReimu() {
    int rc = png.openFLASH((uint8_t *)reimu, sizeof(reimu), pngCallback);
    if (rc == PNG_SUCCESS) {
        Serial.println("Successfully opened png file");
        Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
        tft.startWrite();
        unsigned long dt = millis();
        rc = png.decode(NULL, 0);
        Serial.printf("Decoding took %lu ms\n", millis() - dt);
        tft.endWrite();
    }
}

int pngCallback(PNGDRAW* pDraw) {
    uint16_t lineBuffer[MAX_IMAGE_WIDTH];
    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
    tft.pushImage(0, 0 + pDraw->y, pDraw->iWidth, 1, lineBuffer);
    return 1;
}
#endif
