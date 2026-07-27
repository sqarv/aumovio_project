#include "ui_manager.hpp"
#include <stdio.h>

//-------------------------------- MAIN LAYOUT (240x320)

const rect SLOTS[4] = {
    // x, y, w, h, text_ox, text_oy, color,text_color,text
    {10, 10, 105, 90, 6, 6, ST77XX_WHITE, ST77XX_WHITE, "Preset 1"},   // PRESET 1
    {125, 10, 105, 90, 6, 6, ST77XX_WHITE, ST77XX_WHITE, "Preset 2"},  // PRESET 2
    {10, 156, 105, 90, 6, 6, ST77XX_WHITE, ST77XX_WHITE, "Preset 3"},  // PRESET 3
    {125, 156, 105, 90, 6, 6, ST77XX_WHITE, ST77XX_WHITE, "Preset 4"}, // PRESET 4
};
const uint8_t SLOT_TO_PRESET[4] = {0, 1, 2, 3};

const rect MAIN_BUTTONS[] = {
    {8, 112, 72, 34, 10, 12, ST77XX_GREEN, ST77XX_GREEN, "START"}, // BTN_START
    {86, 112, 68, 34, 10, 12, ST77XX_RED, ST77XX_RED, "STOP"},     // BTN_STOP
    {160, 112, 72, 34, 10, 12, ST77XX_CYAN, ST77XX_CYAN, "PAUSE"}  // BTN_PAUSE
};

//-------------------------------- EDIT LAYOUT

const rect FIELD_RECT[FLD_COUNT] = {
    {20, 34, 60, 34, 0, 0, ST77XX_WHITE, ST77XX_WHITE, ""},   // TON_H
    {90, 34, 60, 34, 0, 0, ST77XX_WHITE, ST77XX_WHITE, ""},   // TON_M
    {160, 34, 60, 34, 0, 0, ST77XX_WHITE, ST77XX_WHITE, ""},  // TON_S
    {20, 84, 60, 34, 0, 0, ST77XX_WHITE, ST77XX_WHITE, ""},   // TOFF_H
    {90, 84, 60, 34, 0, 0, ST77XX_WHITE, ST77XX_WHITE, ""},   // TOFF_M
    {160, 84, 60, 34, 0, 0, ST77XX_WHITE, ST77XX_WHITE, ""},  // TOFF_S
    {70, 134, 100, 34, 0, 0, ST77XX_WHITE, ST77XX_WHITE, ""}, // CYCLES
};

const rect EDIT_BUTTONS[] = {
    {40, 180, 60, 40, 24, 12, ST77XX_WHITE, ST77XX_WHITE, "-"},      // BTN_MINUS
    {140, 180, 60, 40, 24, 12, ST77XX_WHITE, ST77XX_WHITE, "+"},     // BTN_PLUS
    {5, 270, 70, 40, 10, 14, ST77XX_CYAN, ST77XX_CYAN, "<BACK"},     // BTN_BACK
    {85, 270, 70, 40, 10, 14, ST77XX_YELLOW, ST77XX_YELLOW, "MAIN"}, // BTN_MAIN
    {165, 270, 70, 40, 10, 14, ST77XX_CYAN, ST77XX_CYAN, "NEXT>"},   // BTN_NEXT
};

enum MainBtnIdx { BTN_START_IDX = 0, BTN_STOP_IDX, BTN_FREEZE_IDX };
enum EditBtnIdx { BTN_MINUS_IDX = 0, BTN_PLUS_IDX, BTN_BACK_IDX, BTN_MAIN_IDX, BTN_NEXT_IDX };

//-------------------------------- INTERTAN STATE
static Adafruit_ST7789 *s_tft = nullptr;
static timer_preset *s_presets = nullptr;
screen screen_state = SCR_MAIN;
uint8_t editing_preset = 0;
edit_field selected_field = FLD_TON_H;
uint32_t freeze_start_ms = 0;
uint32_t last_dynamic_ms = 0;

//-------------------------------- HELPER FUNCTIONS

bool hit_rect(const rect &r, int x, int y)
{
  return x >= r.x && x <= (r.x + r.w) && y >= r.y && y <= (r.y + r.h);
}

void format_HMS(uint32_t ms, char *buf, size_t buf_len)
{
  uint32_t total_sec = ms / 1000UL;
  uint16_t h = (uint16_t)((total_sec / 3600UL) % 100UL);
  uint8_t m = (uint8_t)((total_sec / 60UL) % 60UL);
  uint8_t s = (uint8_t)(total_sec % 60UL);
  snprintf(buf, buf_len, "%02u:%02u:%02u", h, m, s);
}

void draw_rect(const rect &r)
{
  s_tft->drawRect(r.x, r.y, r.w, r.h, r.color);
  s_tft->setCursor(r.x + r.text_ox, r.y + r.text_oy);
  s_tft->setTextColor(r.text_color);
  s_tft->print(r.text);
}

//-------------------------------- draw MAIN SCREEN

void draw_main_static()
{ // draw static ui of the main screen
  s_tft->fillScreen(ST77XX_BLACK);
  s_tft->setTextSize(1);

  for (uint8_t slot = 0; slot < 4; slot++)
  { // draw s_presets buttons
    draw_rect(SLOTS[slot]);
  }

  for (uint8_t btn = 0; btn < 3; btn++)
  { // draw control buttons
    draw_rect(MAIN_BUTTONS[btn]);
  }
}

void draw_main_dynamic(system_state state)
{
  uint32_t now = (state == SYS_FROZEN) ? freeze_start_ms : millis();
  char buf[9];

  for (uint8_t slot = 0; slot < 4; slot++)
  {
    uint8_t pr_idx = SLOT_TO_PRESET[slot];
    timer_preset &pr = s_presets[pr_idx];
    const rect &r = SLOTS[slot];

    // status circle
    bool relay_on = (state == SYS_RUNNING && pr.state == RELAY_ON && !pr.finished);
    s_tft->fillCircle(r.x + r.w - 14, r.y + 14, 6, relay_on ? ST77XX_GREEN : ST77XX_RED);

    // remaining time (draws black background directly under text)
    uint32_t remain = preset_time_left(pr, now, state);
    format_HMS(remain, buf, sizeof(buf));
    s_tft->setCursor(r.x + 6, r.y + 42);
    s_tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK); // FG, BG
    s_tft->setTextSize(1);
    s_tft->print(buf);

    // Preset status
    s_tft->setCursor(r.x + 6, r.y + 62);
    s_tft->setTextColor(ST77XX_YELLOW, ST77XX_BLACK); // FG, BG

    // clear leftover trailing characters when status string shrinks (e.g. FREEZE -> ON)
    if (state == SYS_IDLE)
      s_tft->print("IDLE  ");
    else if (state == SYS_FROZEN)
      s_tft->print("FREEZE");
    else if (pr.finished)
      s_tft->print("DONE  ");
    else
      s_tft->print(pr.state == RELAY_ON ? "ON    " : "OFF   ");
  }
}

//-------------------------------- draw EDIT SCREEN

void draw_edit_static(uint8_t idx)
{ // draw static ui of the edit screen
  s_tft->fillScreen(ST77XX_BLACK);
  s_tft->setTextSize(2);
  s_tft->setTextColor(ST77XX_WHITE);
  s_tft->setCursor(10, 6);
  s_tft->print("Preset ");
  s_tft->print(idx + 1);

  s_tft->setTextSize(1);
  s_tft->setCursor(20, 24);
  s_tft->print("TON (H:M:S)");
  s_tft->setCursor(20, 74);
  s_tft->print("TOFF (H:M:S)");
  s_tft->setCursor(70, 124);
  s_tft->print("Cicluri (0=infinit)");

  for (uint8_t f = 0; f < FLD_COUNT; f++)
  { // draw field buttons
    draw_rect(FIELD_RECT[f]);
  }

  for (uint8_t btn = 0; btn < 3; btn++)
  { // draw control buttons
    draw_rect(EDIT_BUTTONS[btn]);
  }
}

void draw_edit_dynamic(timer_preset &pr)
{
  static edit_field last_selected = (edit_field)-1;
  char buf[6];

  s_tft->setTextSize(2);
  s_tft->setTextColor(ST77XX_WHITE, ST77XX_BLACK); // overwrite text background directly

  for (uint8_t f = 0; f < FLD_COUNT; f++)
  {
    const rect &r = FIELD_RECT[f];
    edit_field field = (edit_field)f;

    // redraw borders ONLY if the selected field changed
    if (selected_field != last_selected)
    {
      uint16_t border_color = (field == selected_field) ? ST77XX_YELLOW : ST77XX_WHITE;
      s_tft->drawRect(r.x, r.y, r.w, r.h, border_color);
    }

    switch (f)
    {
    case FLD_TON_H:
      snprintf(buf, sizeof(buf), "%02u", pr.ton.h);
      break;
    case FLD_TON_M:
      snprintf(buf, sizeof(buf), "%02u", pr.ton.m);
      break;
    case FLD_TON_S:
      snprintf(buf, sizeof(buf), "%02u", pr.ton.s);
      break;
    case FLD_TOFF_H:
      snprintf(buf, sizeof(buf), "%02u", pr.toff.h);
      break;
    case FLD_TOFF_M:
      snprintf(buf, sizeof(buf), "%02u", pr.toff.m);
      break;
    case FLD_TOFF_S:
      snprintf(buf, sizeof(buf), "%02u", pr.toff.s);
      break;
    case FLD_CYCLES:
      snprintf(buf, sizeof(buf), "%3u", pr.cycles);
      break;
    default:
      buf[0] = '\0';
      break;
    }

    s_tft->setCursor(r.x + 8, r.y + 8);
    s_tft->print(buf);
  }

  last_selected = selected_field;
}

//-------------------------------- UI HANDLERS

void ui_init(Adafruit_ST7789 *tftPtr, timer_preset *channelsPtr)
{
  s_tft = tftPtr;
  s_presets = channelsPtr;
}

void ui_draw_main(system_state state)
{
  screen_state = SCR_MAIN;
  draw_main_static();
  draw_main_dynamic(state);
}

void ui_tick(system_state state)
{ // refresh presets status when something changed
  if (screen_state != SCR_MAIN)
    return;

  uint32_t now = millis();
  if (now - last_dynamic_ms < DYNAMIC_REFRESH_TIME)
    return;
  last_dynamic_ms = now;
  draw_main_dynamic(state);
}

void ui_handle_tap(int x, int y, system_state &state)
{
  if (screen_state == SCR_MAIN) // check only main screen buttons
  {
    // 1. check main control buttons (START, STOP, PAUSE)
    for (uint8_t i = 0; i < sizeof(MAIN_BUTTONS) / sizeof(MAIN_BUTTONS[0]); i++) 
    {
      if (hit_rect(MAIN_BUTTONS[i], x, y)) 
      {
        switch (i) 
        {
          case BTN_START_IDX:
            if (state == SYS_IDLE) {
              for (uint8_t p = 0; p < 4; p++) preset_on(s_presets[p]);
              state = SYS_RUNNING;
            }
            break;

          case BTN_STOP_IDX:
            for (uint8_t p = 0; p < 4; p++) preset_off(s_presets[p]);
            state = SYS_IDLE;
            break;

          case BTN_FREEZE_IDX:
            if (state == SYS_RUNNING) {
              freeze_start_ms = millis();
              state = SYS_FROZEN;
            } else if (state == SYS_FROZEN) {
              uint32_t paused = millis() - freeze_start_ms;
              for (uint8_t p = 0; p < 4; p++) preset_shift_freeze(s_presets[p], paused);
              state = SYS_RUNNING;
            }
            break;
        }
        draw_main_dynamic(state);
        return;
      }
    }

    // 2. check preset slots
    for (uint8_t slot = 0; slot < 4; slot++) 
    {
      if (hit_rect(SLOTS[slot], x, y)) 
      {
        editing_preset = SLOT_TO_PRESET[slot];
        selected_field = FLD_TON_H;
        screen_state = SCR_EDIT;
        draw_edit_static(editing_preset);
        draw_edit_dynamic(s_presets[editing_preset]);
        return;
      }
    }
  } 
  else // check only edit screen buttons
  {
    timer_preset &pr = s_presets[editing_preset];

    // 1. check input fields
    for (uint8_t f = 0; f < FLD_COUNT; f++) 
    {
      if (hit_rect(FIELD_RECT[f], x, y)) 
      {
        selected_field = (edit_field)f;
        draw_edit_dynamic(pr);
        return;
      }
    }

    // 2. check edit control buttons (+, -, BACK, MAIN, NEXT)
    for (uint8_t i = 0; i < sizeof(EDIT_BUTTONS) / sizeof(EDIT_BUTTONS[0]); i++) 
    {
      if (hit_rect(EDIT_BUTTONS[i], x, y)) 
      {
        switch (i) 
        {
          case BTN_MINUS_IDX:
            adjust_preset_field(pr, selected_field, -1);
            draw_edit_dynamic(pr);
            break;

          case BTN_PLUS_IDX:
            adjust_preset_field(pr, selected_field, +1);
            draw_edit_dynamic(pr);
            break;

          case BTN_BACK_IDX:
            editing_preset = (uint8_t)((editing_preset + 3) % 4);
            selected_field = FLD_TON_H;
            draw_edit_static(editing_preset);
            draw_edit_dynamic(s_presets[editing_preset]);
            break;

          case BTN_NEXT_IDX:
            editing_preset = (uint8_t)((editing_preset + 1) % 4);
            selected_field = FLD_TON_H;
            draw_edit_static(editing_preset);
            draw_edit_dynamic(s_presets[editing_preset]);
            break;

          case BTN_MAIN_IDX:
            screen_state = SCR_MAIN;
            draw_main_static();
            draw_main_dynamic(state);
            break;
        }
        return;
      }
    }
  }
}

//--------------------------------