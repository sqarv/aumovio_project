#pragma once
#include <Adafruit_ST7789.h>
#include "timer_preset.hpp"
#include "config.hpp"

//-------------------------------- DATA STRUCTURES DEFINITIONS

struct rect {
    int8_t x, y, w, h, text_ox, text_oy;
    uint16_t color,text_color;
    char text[10];
};

enum screen : uint8_t { SCR_MAIN = 0, SCR_EDIT = 1 };

//-------------------------------- FUNCTION DECLARATIONS

void ui_init(Adafruit_ST7789 *tftPtr, timer_preset *presetsPtr);
void ui_draw_main(system_state state);      // draw MAIN screen
void ui_handle_tap(int x, int y, system_state &state); // tap route to the current screen
void ui_tick(system_state state);

//--------------------------------