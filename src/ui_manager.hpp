#pragma once
#include <Adafruit_ST7789.h>
#include "timer_preset.hpp"
#include "config.hpp"
#include "theme.hpp"

//-------------------------------- DATA STRUCTURES DEFINITIONS

struct ui_label {
  int16_t x;
  int16_t y;
  uint8_t font_size;
  uint16_t color;
  const char *text;
};

struct rect {
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
  int16_t text_ox;
  int16_t text_oy;
  uint8_t text_size;
  int16_t radius;
  uint16_t color;        // Main body/fill color (or border color if not filled)
  uint16_t border_color; // Border color (used when filled)
  uint16_t text_color;   // Text foreground color
  bool fill;             // True to render solid fill
  const char *text;
};

enum screen : uint8_t
{
    SCR_MAIN = 0,
    SCR_EDIT = 1
};

//-------------------------------- FUNCTION DECLARATIONS

void ui_init(Adafruit_ST7789 *tftPtr, timer_preset *presetsPtr);
void ui_draw_main(system_state state);                 // draw MAIN screen
bool ui_handle_tap(int x, int y, system_state &state); // tap route to the current screen
void ui_tick(system_state state);

//--------------------------------