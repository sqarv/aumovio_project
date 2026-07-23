#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define TFT_CS   10 // chip select pin
#define TFT_DC    9 // data/command pin
#define TFT_RST   8 // reset pin

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  tft.init(240, 320);
  tft.setRotation(1);
  tft.invertDisplay(0);
  tft.fillScreen(ST77XX_BLACK);

  tft.fillRect(10, 10, 100, 60, ST77XX_BLUE);
  tft.setCursor(20, 120);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.print("Salut");
}

void loop(){
  
}