#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>
#include "config.hpp"

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RESET);
XPT2046_Touchscreen touch(T_CS);

// Global State
ScreenState currentScreen = STATE_MAIN;
uint8_t activePresetIdx = 0; // 0 to 3 for Custom 1-4
unsigned long lastTouchTime = 0;

// Preset Data Storage
Preset presets[4] = {
  {0, 18, 45, 0, 10, 0, 10, true},
  {1, 25, 30, 0, 15, 0, 5,  true},
  {0, 7,  1,  0, 5,  0, 3,  false},
  {0, 33, 55, 0, 12, 0, 8,  false}
};

// Prototype declarations
void drawMainScreen();
void drawSettingsScreen();
void handleTouch(int x, int y);
void triggerBuzzer();

void setup() {
  pinMode(BUZZER, OUTPUT);
  pinMode(BRIGHTNESS, OUTPUT);
  analogWrite(BRIGHTNESS, 255); // Full brightness

  tft.init(240, 320);
  tft.setRotation(1); // 320x240 Landscape
  tft.invertDisplay(0);
  touch.begin();
  touch.setRotation(1);

  drawMainScreen();
}

void loop() {
  if (touch.touched() && (millis() - lastTouchTime > 250)) { // 250ms debounce
    TS_Point p = touch.getPoint();
    
    // Map raw touch coordinates to screen pixels
    int x = map(p.x, TS_MINX, TS_MAXX, 0, SCREEN_W);
    int y = map(p.y, TS_MINY, TS_MAXY, 0, SCREEN_H);

    handleTouch(x, y);
    lastTouchTime = millis();
  }
}

// -------------------------------------------------------------
// UI DRAWING FUNCTIONS
// -------------------------------------------------------------

void drawHeader(const char* title) {
  tft.fillRect(0, 0, SCREEN_W, 30, COLOR_HEADER);
  tft.setTextColor(COLOR_TEXT, COLOR_HEADER);
  tft.setTextSize(2);
  tft.setCursor(20, 7);
  tft.print(title);
}

void drawMainScreen() {
  currentScreen = STATE_MAIN;
  tft.fillScreen(COLOR_BG);
  drawHeader("SYSTEM CONTROL PANEL");

  // Global FREEZE Button
  tft.fillRoundRect(30, 35, 260, 35, 6, COLOR_BLUE);
  tft.setCursor(110, 45);
  tft.setTextColor(COLOR_TEXT, COLOR_BLUE);
  tft.print("FREEZE");

  // 2x2 Preset Grid
  const char* titles[4] = {"CUSTOM 1", "CUSTOM 2", "CUSTOM 3", "CUSTOM 4"};
  int coords[4][2] = {{10, 80}, {165, 80}, {10, 155}, {165, 155}};

  for (int i = 0; i < 4; i++) {
    int x = coords[i][0];
    int y = coords[i][1];
    
    tft.fillRoundRect(x, y, 145, 65, 6, COLOR_CARD);
    tft.setTextColor(COLOR_TEXT, COLOR_CARD);
    tft.setTextSize(1);
    tft.setCursor(x + 40, y + 8);
    tft.print(titles[i]);

    // Timer display
    tft.setTextSize(2);
    tft.setCursor(x + 25, y + 25);
    char buf[9];
    sprintf(buf, "%02d:%02d:%02d", presets[i].onH, presets[i].onM, presets[i].onS);
    tft.print(buf);

    // Active indicator dot
    uint16_t dotColor = presets[i].active ? COLOR_ORANGE : COLOR_MUTED;
    tft.fillCircle(x + 72, y + 52, 4, dotColor);
  }
}

void drawSettingsScreen() {
  currentScreen = STATE_SETTINGS;
  tft.fillScreen(COLOR_BG);

  char titleBuf[25];
  sprintf(titleBuf, "PRESET %d SETTINGS", activePresetIdx + 1);
  drawHeader(titleBuf);

  // ON Period Display
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextSize(1);
  tft.setCursor(120, 40);
  tft.print("ON PERIOD");
  
  tft.setTextSize(3);
  tft.setCursor(50, 55);
  char buf[9];
  sprintf(buf, "%02d:%02d:%02d", presets[activePresetIdx].onH, presets[activePresetIdx].onM, presets[activePresetIdx].onS);
  tft.print(buf);

  // OFF Period Display
  tft.setTextSize(1);
  tft.setCursor(115, 95);
  tft.print("OFF PERIOD");

  tft.setTextSize(3);
  tft.setCursor(50, 110);
  sprintf(buf, "%02d:%02d:%02d", presets[activePresetIdx].offH, presets[activePresetIdx].offM, presets[activePresetIdx].offS);
  tft.print(buf);

  // Repeat Cycles
  tft.setTextSize(1);
  tft.setCursor(110, 145);
  tft.print("REPEAT CYCLES");
  tft.setTextSize(2);
  tft.setCursor(145, 160);
  tft.print(presets[activePresetIdx].cycles);

  // Bottom Navigation Buttons
  tft.fillRoundRect(10, 185, 145, 22, 4, COLOR_CARD);
  tft.setTextColor(COLOR_TEXT, COLOR_CARD);
  tft.setTextSize(1);
  tft.setCursor(45, 192);
  tft.print("< PREV");

  tft.fillRoundRect(165, 185, 145, 22, 4, COLOR_CARD);
  tft.setCursor(205, 192);
  tft.print("NEXT >");

  tft.fillRoundRect(10, 212, 300, 24, 4, COLOR_BLUE);
  tft.setTextColor(COLOR_TEXT, COLOR_BLUE);
  tft.setTextSize(2);
  tft.setCursor(95, 216);
  tft.print("MAIN SCREEN");
}

// -------------------------------------------------------------
// TOUCH & NAVIGATION LOGIC
// -------------------------------------------------------------

void triggerBuzzer() {
  digitalWrite(BUZZER, HIGH);
  delay(30);
  digitalWrite(BUZZER, LOW);
}

void handleTouch(int x, int y) {
  triggerBuzzer();

  if (currentScreen == STATE_MAIN) {
    // Check preset button taps
    if (x >= 10 && x <= 155 && y >= 80 && y <= 145)      { activePresetIdx = 0; drawSettingsScreen(); }
    else if (x >= 165 && x <= 310 && y >= 80 && y <= 145) { activePresetIdx = 1; drawSettingsScreen(); }
    else if (x >= 10 && x <= 155 && y >= 155 && y <= 220){ activePresetIdx = 2; drawSettingsScreen(); }
    else if (x >= 165 && x <= 310 && y >= 155 && y <= 220){ activePresetIdx = 3; drawSettingsScreen(); }
  } 
  else if (currentScreen == STATE_SETTINGS) {
    // PREV Button
    if (x >= 10 && x <= 155 && y >= 185 && y <= 207) {
      activePresetIdx = (activePresetIdx == 0) ? 3 : activePresetIdx - 1; // Wraps 1 -> 4
      drawSettingsScreen();
    }
    // NEXT Button
    else if (x >= 165 && x <= 310 && y >= 185 && y <= 207) {
      activePresetIdx = (activePresetIdx + 1) % 4; // Wraps 4 -> 1
      drawSettingsScreen();
    }
    // MAIN SCREEN Button
    else if (x >= 10 && x <= 310 && y >= 212 && y <= 236) {
      drawMainScreen();
    }
  }
}