#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>
#include <string.h>

#include "config.hpp"
#include "timer_channel.hpp"
#include "ui_manager.hpp"

// INIT
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST); // display object
XPT2046_Touchscreen ts(T_CS); // touch object

TimerChannel presets[4] = {
  { "Custom 1", RELAY_PINS[0], {0,0,5}, {0,0,5}, 3, PH_ON, 0, 0, 0, false },
  { "Custom 2", RELAY_PINS[1], {0,0,5}, {0,0,5}, 3, PH_ON, 0, 0, 0, false },
  { "Custom 3", RELAY_PINS[2], {0,0,5}, {0,0,5}, 3, PH_ON, 0, 0, 0, false },
  { "Custom 4", RELAY_PINS[3], {0,0,5}, {0,0,5}, 3, PH_ON, 0, 0, 0, false },
};

SystemState systemState = SYS_IDLE;

// FUNCTIONS
bool readTap(int &x, int &y) {
  if (!ts.touched()) return false;
  TS_Point p = ts.getPoint();
  
  String text = String("X: ") + String(p.x) + String(" Y: ") + String(p.y) + String(" Preassure: ") + String(p.z);
  Serial.println(text); // touch debbuging
  
  // map touch location to screen size
  x = map(p.x, 0, 240, 281, 3700);
  y = map(p.y, 0, 320, 321, 3720);
  
  // limit touch coordinates to screen boundaries
  x = constrain(x,0,320);
  y = constrain(y,0,240);
  
  return true;
}

// SETUP
void setup() {
  Serial.begin(9600);
  
  // set relays
  for (uint8_t i = 0; i < 4; i++) {
    pinMode(presets[i].relayPin, OUTPUT);
    digitalWrite(presets[i].relayPin, LOW);
  }
  
  // init display
  tft.init(SCREEN_W,SCREEN_H);
  tft.invertDisplay(0); // non-inverted colors
  tft.setRotation(1); // landscape
  
  // init touch
  ts.begin();
  ts.setRotation(0);

  uiInit(&tft, presets);
  uiDrawMain(systemState);
}

// LOOP
void loop() {
  uint32_t now = millis();

  for (uint8_t i = 0; i < 4; i++) {
    channelUpdate(presets[i], systemState, now);
  }

  int x, y;
  if (readTap(x, y)) {
    uiHandleTap(x, y, systemState);
    delay(180); // debounce simplu
  }

  uiTick(systemState);
}
