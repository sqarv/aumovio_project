#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Pins Configuration
#define TFT_CS     10
#define TFT_RESET   9    
#define TFT_DC      8
#define T_CS        7
#define BUZZER      6
#define BRIGHTNESS  5

#define RELAY_1 A1
#define RELAY_2 A2
#define RELAY_3 A3
#define RELAY_4 A4

// Display Resolution
#define SCREEN_W 320
#define SCREEN_H 240

// Touch Calibration Thresholds (Calibrate these for your hardware)
#define TS_MINX 200
#define TS_MINY 200
#define TS_MAXX 3800
#define TS_MAXY 3800

// Color Palette (RGB565 format)
#define COLOR_BG        0x10A2 // Deep Charcoal
#define COLOR_HEADER    0x0000 // Black
#define COLOR_CARD      0x29A6 // Card Fill
#define COLOR_BLUE      0x03FF // Main Action Blue
#define COLOR_TEXT      0xFFFF // White Text
#define COLOR_MUTED     0x8410 // Muted Text/Gray
#define COLOR_ORANGE    0xFD20 // Active Indicator

// Data Structures
struct Preset {
  uint8_t onH, onM, onS;
  uint8_t offH, offM, offS;
  uint8_t cycles;
  bool active;
};

enum ScreenState {
  STATE_MAIN,
  STATE_SETTINGS
};

#endif