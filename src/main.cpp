#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define TFT_CS   10
#define TFT_DC    9
#define TFT_RST   8

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  tft.init(240, 320);          // Initialize ST7789 240x320 hardware
  tft.setRotation(1);          // 1 = Landscape (320x240)
  tft.fillScreen(ST77XX_MAGENTA); // Clears the entire display memory

  tft.fillRect(10, 10, 100, 60, ST77XX_BLUE);
  tft.setCursor(20, 120);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.print("System Ready 320x240");
}

void loop() {}