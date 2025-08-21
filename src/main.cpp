#include "reimu.h"
#include "constants.h"

// Bluetooth

// PNG decoding
#include <PNGdec.h>
PNG png; // PNG decoder instance

// Touchscreen and display
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
SPIClass mySpi = SPIClass(VSPI);
TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);


// Forward declarations
void checkTouchScreen();
void drawReimu();
int pngCallback(PNGDRAW* pDraw);

void setup()
{
    Serial.begin(115200);
    
    // Initialise the display
    tft.init();
    tft.setRotation(1); //This is the display in landscape
    
    // Initialise the touchscreen
    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(mySpi);
    ts.setRotation(1);
    
    // Clear the screen before writing to it
    tft.fillScreen(TFT_BLACK);
    drawReimu();
}

void loop()
{
    checkTouchScreen();
}

unsigned long lastTouchTime = 0;
void checkTouchScreen() {
    unsigned long currentMillis = millis();

    if (ts.touched() && (currentMillis - lastTouchTime > DEBOUNCE_TIME)) {
        lastTouchTime = currentMillis;
        TS_Point p = ts.getPoint();
        Serial.printf("Touch detected at (%d, %d)\n", p.x, p.y);
    }
}

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
