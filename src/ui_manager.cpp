#include "ui_manager.hpp"
#include <stdio.h>

//-------------------------------- MAIN LAYOUT (240x320)

//slots
 const rect SLOTS[4] = {
  //x, y, w, h, text_ox, text_oy, color,text_color,text
  {10 , 10 , 105 , 90 , 6 , 6 , ST77XX_WHITE, ST77XX_WHITE, "Preset 1"}, // PRESET 1
  {125, 10, 105, 90 , 6 , 6 , ST77XX_WHITE, ST77XX_WHITE, "Preset 2"}, // PRESET 2
  {10, 156, 105, 90 , 6 , 6 , ST77XX_WHITE, ST77XX_WHITE, "Preset 3"}, // PRESET 3
  {125,156, 105, 90 , 6 , 6 , ST77XX_WHITE, ST77XX_WHITE, "Preset 4"}, // PRESET 4
};
 const uint8_t SLOT_TO_PRESET[4] = {0, 1, 2, 3};

//main buttons
 const rect MAIN_BUTTONS[] = {
  {8,  112, 72, 34 , 10 , 12 , ST77XX_GREEN, ST77XX_GREEN, "START"}, // BTN_START
  {86, 112, 68, 34 , 10 , 12 , ST77XX_RED, ST77XX_RED, "STOP"}, // BTN_STOP
  {160,112, 72, 34 , 10 , 12 , ST77XX_CYAN, ST77XX_CYAN, "PAUSE"} // BTN_PAUSE
 };

//-------------------------------- EDIT LAYOUT

//fields
 const rect FIELD_RECT[FLD_COUNT] = {
  {20,  34, 60, 34, 0 , 0 , ST77XX_WHITE, ST77XX_WHITE, ""}, // TON_H
  {90,  34, 60, 34, 0 , 0 , ST77XX_WHITE, ST77XX_WHITE, ""}, // TON_M
  {160, 34, 60, 34, 0 , 0 , ST77XX_WHITE, ST77XX_WHITE, ""}, // TON_S
  {20,  84, 60, 34, 0 , 0 , ST77XX_WHITE, ST77XX_WHITE, ""}, // TOFF_H
  {90,  84, 60, 34, 0 , 0 , ST77XX_WHITE, ST77XX_WHITE, ""}, // TOFF_M
  {160, 84, 60, 34, 0 , 0 , ST77XX_WHITE, ST77XX_WHITE, ""}, // TOFF_S
  {70, 134, 100,34, 0 , 0 , ST77XX_WHITE, ST77XX_WHITE, ""}, // CYCLES
};

//edit buttons
const rect EDIT_BUTTONS[] = {
  {40, 180, 60, 40, 24 , 12 , ST77XX_WHITE, ST77XX_WHITE, "-"}, // BTN_MINUS
  {140,180, 60, 40, 24 , 12 , ST77XX_WHITE, ST77XX_WHITE, "+"}, // BTN_PLUS
  {5,  270, 70, 40, 10 , 14 , ST77XX_CYAN, ST77XX_CYAN, "<BACK"}, // BTN_BACK
  {85, 270, 70, 40, 10 , 14 , ST77XX_YELLOW, ST77XX_YELLOW, "MAIN"}, // BTN_MAIN
  {165,270, 70, 40, 10 , 14 , ST77XX_CYAN, ST77XX_CYAN, "NEXT>"}, // BTN_NEXT
};

//-------------------------------- INTERTAN STATE
 Adafruit_ST7789 *tft = nullptr;
 timer_preset *presets = nullptr;
 screen screen_state = SCR_MAIN;
 uint8_t editing_preset = 0;
 edit_field selected_field = FLD_TON_H;
 uint32_t freeze_start_ms = 0;
 uint32_t last_dynamic_ms = 0;

//-------------------------------- HELPER FUNCTIONS

 bool hit_rect(const rect &r, int x, int y) {
  return x >= r.x && x <= (r.x + r.w) && y >= r.y && y <= (r.y + r.h);
}

 void format_HMS(uint32_t ms, char *buf, size_t buf_len) {
  uint32_t total_sec = ms / 1000UL;
  uint16_t h = (uint16_t)((total_sec / 3600UL) % 100UL);
  uint8_t m = (uint8_t)((total_sec / 60UL) % 60UL);
  uint8_t s = (uint8_t)(total_sec % 60UL);
  snprintf(buf, buf_len, "%02u:%02u:%02u", h, m, s);
}

void draw_rect(const rect &r) {
    tft->drawRect(r.x, r.y, r.w, r.h, r.color);
    tft->setCursor(r.x + r.text_ox, r.y + r.text_oy);
    tft->setTextColor(r.text_color);
    tft->print(r.text);
}

//-------------------------------- draw MAIN SCREEN
 void draw_main_static() { // draw static ui of the main screen
  tft->fillScreen(ST77XX_BLACK);
  tft->setTextSize(1);
  
  for (uint8_t slot = 0; slot < 4; slot++) { // draw presets buttons
    draw_rect(SLOTS[slot]);
  }
  
  for (uint8_t btn = 0; btn < 3; btn++){ // draw control buttons
    draw_rect(MAIN_BUTTONS[btn]);
  }
}

 void drawMainDynamic(system_state state) {
  uint32_t now = (state == SYS_FROZEN) ? freeze_start_ms : millis();
  char buf[9];
  for (uint8_t slot = 0; slot < 4; slot++) {
    uint8_t chIdx = SLOT_TO_PRESET[slot];
    timer_preset &pr = presets[chIdx];
    const rect &r = SLOTS[slot];

    bool relayOn = (state == SYS_RUNNING && pr.state == RELAY_ON && !pr.finished);
    tft->fillCircle(r.x + r.w - 14, r.y + 14, 6, relayOn ? ST77XX_GREEN : ST77XX_RED);

    tft->fillRect(r.x + 4, r.y + 40, r.w - 8, 14, ST77XX_BLACK);
    uint32_t remain = preset_time_left(pr, now, state);
    format_HMS(remain, buf, sizeof(buf));
    tft->setCursor(r.x + 6, r.y + 42);
    tft->setTextColor(ST77XX_WHITE);
    tft->setTextSize(1);
    tft->print(buf);

    tft->fillRect(r.x + 4, r.y + 60, r.w - 8, 12, ST77XX_BLACK);
    tft->setCursor(r.x + 6, r.y + 62);
    tft->setTextColor(ST77XX_YELLOW);
    if (state == SYS_IDLE) tft->print("IDLE");
    else if (state == SYS_FROZEN) tft->print("FREEZE");
    else if (pr.finished) tft->print("DONE");
    else tft->print(pr.state == RELAY_ON ? "ON" : "OFF");
  }
}

//-------------------------------- draw EDIT SCREEN
 void draw_edit_static(uint8_t idx) { // draw static ui of the edit screen
  tft->fillScreen(ST77XX_BLACK);
  tft->setTextSize(2);
  tft->setTextColor(ST77XX_WHITE);
  tft->setCursor(10, 6);
  tft->print("Custom ");
  tft->print(idx + 1);

  tft->setTextSize(1);
  tft->setCursor(20, 24);
  tft->print("TON (H:M:S)");
  tft->setCursor(20, 74);
  tft->print("TOFF (H:M:S)");
  tft->setCursor(70, 124);
  tft->print("Cicluri (0=infinit)");
  
  for (uint8_t f = 0; f < FLD_COUNT; f++) { // draw field buttons
    draw_rect(FIELD_RECT[f]);
  }
  
  for (uint8_t btn = 0; btn < 3; btn++){ // draw control buttons
    draw_rect(EDIT_BUTTONS[btn]);
  }
}

 void drawEditDynamic(timer_preset &pr) {
  char buf[6];
  for (uint8_t f = 0; f < FLD_COUNT; f++) {
    const rect &r = FIELD_RECT[f];
    uint16_t color = ((edit_field)f == selected_field) ? ST77XX_YELLOW : ST77XX_WHITE;
    tft->drawRect(r.x, r.y, r.w, r.h, color);
    tft->fillRect(r.x + 2, r.y + 2, r.w - 4, r.h - 4, ST77XX_BLACK);
    tft->setTextSize(2);
    tft->setTextColor(ST77XX_WHITE);
    tft->setCursor(r.x + 8, r.y + 8);
    switch (f) {
      case FLD_TON_H:  snprintf(buf, sizeof(buf), "%02u", pr.ton.h);  break;
      case FLD_TON_M:  snprintf(buf, sizeof(buf), "%02u", pr.ton.m);  break;
      case FLD_TON_S:  snprintf(buf, sizeof(buf), "%02u", pr.ton.s);  break;
      case FLD_TOFF_H: snprintf(buf, sizeof(buf), "%02u", pr.toff.h); break;
      case FLD_TOFF_M: snprintf(buf, sizeof(buf), "%02u", pr.toff.m); break;
      case FLD_TOFF_S: snprintf(buf, sizeof(buf), "%02u", pr.toff.s); break;
      case FLD_CYCLES: snprintf(buf, sizeof(buf), "%3u",  pr.cycles); break;
      default: buf[0] = '\0'; break;
    }
    tft->print(buf);
  }
}

//-------------------------------- UI HANDLERS

void uiInit(Adafruit_ST7789 *tftPtr, timer_preset *channelsPtr) {
  tft = tftPtr;
  presets = channelsPtr;
}

void uiDrawMain(system_state state) {
  screen_state = SCR_MAIN;
  draw_main_static();
  drawMainDynamic(state);
}

void uiTick(system_state state) {
  if (screen_state != SCR_MAIN) return;
  uint32_t now = millis();
  if (now - last_dynamic_ms < 500) return;
  last_dynamic_ms = now;
  drawMainDynamic(state);
}

void uiHandleTap(int x, int y, system_state &state) {
  if (screen_state == SCR_MAIN) {
    if (hit_rect(BTN_START, x, y)) {
      if (state == SYS_IDLE) {
        for (uint8_t i = 0; i < 4; i++) preset_on(presets[i]);
        state = SYS_RUNNING;
      }
      drawMainDynamic(state);
      return;
    }
    if (hit_rect(BTN_STOP, x, y)) {
      for (uint8_t i = 0; i < 4; i++) preset_off(presets[i]);
      state = SYS_IDLE;
      drawMainDynamic(state);
      return;
    }
    if (hit_rect(BTN_FREEZE, x, y)) {
      if (state == SYS_RUNNING) {
        freeze_start_ms = millis();
        state = SYS_FROZEN;
      } else if (state == SYS_FROZEN) {
        uint32_t paused = millis() - freeze_start_ms;
        for (uint8_t i = 0; i < 4; i++) preset_shift_freeze(presets[i], paused);
        state = SYS_RUNNING;
      }
      drawMainDynamic(state);
      return;
    }
    for (uint8_t slot = 0; slot < 4; slot++) {
      if (hit_rect(SLOTS[slot], x, y)) {
        editing_preset = SLOT_TO_PRESET[slot];
        selected_field = FLD_TON_H;
        screen_state = SCR_EDIT;
        draw_edit_static(editing_preset);
        drawEditDynamic(presets[editing_preset]);
        return;
      }
    }
  } else { // SCR_EDIT
    timer_preset &pr = presets[editing_preset];
    for (uint8_t f = 0; f < FLD_COUNT; f++) {
      if (hit_rect(FIELD_RECT[f], x, y)) {
        selected_field = (edit_field)f;
        drawEditDynamic(pr);
        return;
      }
    }
    if (hit_rect(BTN_MINUS, x, y)) { adjust_preset_field(pr, selected_field, -1); drawEditDynamic(pr); return; }
    if (hit_rect(BTN_PLUS,  x, y)) { adjust_preset_field(pr, selected_field, +1); drawEditDynamic(pr); return; }
    if (hit_rect(BTN_BACK, x, y)) {
      editing_preset = (uint8_t)((editing_preset + 3) % 4);
      selected_field = FLD_TON_H;
      draw_edit_static(editing_preset);
      drawEditDynamic(presets[editing_preset]);
      return;
    }
    if (hit_rect(BTN_NEXT, x, y)) {
      editing_preset = (uint8_t)((editing_preset + 1) % 4);
      selected_field = FLD_TON_H;
      draw_edit_static(editing_preset);
      drawEditDynamic(presets[editing_preset]);
      return;
    }
    if (hit_rect(BTN_MAIN2, x, y)) {
      screen_state = SCR_MAIN;
      draw_main_static();
      drawMainDynamic(state);
      return;
    }
  }
}

//--------------------------------