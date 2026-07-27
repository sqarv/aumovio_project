#include "ui_manager.hpp"
#include <stdio.h>

//-------------------------------- MAIN LAYOUT (240x320)

const rect SLOTS[4] = {
    {10, 35, 105, 100, 8, -6, 2, 5, UI_COLOR_P1, UI_COLOR_P1, UI_COLOR_P1, false, "P1"},
    {130, 35, 105, 100, 8, -6, 2, 5, UI_COLOR_P2, UI_COLOR_P2, UI_COLOR_P2, false, "P2"},
    {10, 150, 105, 100, 8, -6, 2, 5, UI_COLOR_P3, UI_COLOR_P3, UI_COLOR_P3, false, "P3"},
    {130, 150, 105, 100, 8, -6, 2, 5, UI_COLOR_P4, UI_COLOR_P4, UI_COLOR_P4, false, "P4"}
};
const uint8_t SLOT_TO_PRESET[4] = {0, 1, 2, 3};

const rect MAIN_BUTTONS[] = {
    {8, 265, 72, 50, 6, 17, 2, 6, UI_COLOR_ACTIVE, UI_COLOR_ACTIVE_BORDER, UI_COLOR_FG, true, "START"},
    {86, 265, 68, 50, 8, 17, 2, 6, UI_COLOR_INACTIVE, UI_COLOR_INACTIVE_BORDER, UI_COLOR_FG, true, "STOP"},
    {160, 265, 72, 50, 6, 17, 2, 6, UI_COLOR_WARN, UI_COLOR_WARN_BORDER, UI_COLOR_FG, true, "PAUSE"}
};

const ui_label MAIN_LABELS[] = {
    {40, 6, 2, UI_COLOR_FG, "MAIN DASHBOARD"}
};
const uint8_t MAIN_LABEL_COUNT = sizeof(MAIN_LABELS) / sizeof(MAIN_LABELS[0]);

enum MainBtnIdx { BTN_START_IDX = 0, BTN_STOP_IDX, BTN_FREEZE_IDX, MAIN_BTN_COUNT };

//-------------------------------- EDIT LAYOUT

const rect FIELD_RECT[FLD_COUNT] = {
    {10, 46, 64, 32, 0, 0, 2, 6, UI_COLOR_P1, UI_COLOR_P1, UI_COLOR_FG, false, ""},
    {88, 46, 64, 32, 0, 0, 2, 6, UI_COLOR_P1, UI_COLOR_P1, UI_COLOR_FG, false, ""},
    {166, 46, 64, 32, 0, 0, 2, 6, UI_COLOR_P1, UI_COLOR_P1, UI_COLOR_FG, false, ""},
    {10, 104, 64, 32, 0, 0, 2, 6, UI_COLOR_P2, UI_COLOR_P2, UI_COLOR_FG, false, ""},
    {88, 104, 64, 32, 0, 0, 2, 6, UI_COLOR_P2, UI_COLOR_P2, UI_COLOR_FG, false, ""},
    {166, 104, 64, 32, 0, 0, 2, 6, UI_COLOR_P2, UI_COLOR_P2, UI_COLOR_FG, false, ""},
    {60, 162, 120, 32, 0, 0, 2, 6, UI_COLOR_FG, UI_COLOR_FG, UI_COLOR_FG, false, ""}
};

const rect EDIT_BUTTONS[] = {
    {30,  202, 75, 42, 30, 13, 2, 6, UI_COLOR_FG, UI_COLOR_FG, UI_COLOR_FG, false, "-"},
{135, 202, 75, 42, 30, 13, 2, 6, UI_COLOR_FG, UI_COLOR_FG, UI_COLOR_FG, false, "+"},
    {5, 256, 70, 55, 6, 20, 2, 6, UI_COLOR_NEUTRAL, UI_COLOR_NEUTRAL_BORDER, UI_COLOR_FG, true, "<BACK"},
    {85, 256, 70, 55, 11, 20, 2, 6, UI_COLOR_WARN, UI_COLOR_WARN_BORDER, UI_COLOR_FG, true, "MAIN"},
    {165, 256, 70, 55, 6, 20, 2, 6, UI_COLOR_NEUTRAL, UI_COLOR_NEUTRAL_BORDER, UI_COLOR_FG, true, "NEXT>"}
};

const ui_label EDIT_LABELS[] = {
    {10, 26, 2, UI_COLOR_FG, "TON (H:M:S)"},
    {10, 84, 2, UI_COLOR_FG, "TOFF (H:M:S)"},
    {30, 142, 2, UI_COLOR_FG, "Cicluri (0=inf)"}
};
const uint8_t EDIT_LABEL_COUNT = sizeof(EDIT_LABELS) / sizeof(EDIT_LABELS[0]);

enum EditBtnIdx { BTN_MINUS_IDX = 0, BTN_PLUS_IDX, BTN_BACK_IDX, BTN_MAIN_IDX, BTN_NEXT_IDX, EDIT_BTN_COUNT };

//-------------------------------- INTERTAN STATE
static Adafruit_ST7789 *s_tft = nullptr;
static timer_preset *s_presets = nullptr;
screen screen_state = SCR_MAIN;
uint8_t editing_preset = 0;
edit_field selected_field = FLD_TON_H;
edit_field last_selected = (edit_field)-1;
uint32_t freeze_start_ms = 0;
uint32_t last_dynamic_ms = 0;

//-------------------------------- HELPER FUNCTIONS

void clear_label(const ui_label &lbl) {
  uint16_t char_w = 6 * lbl.font_size;
  uint16_t char_h = 8 * lbl.font_size;
  uint16_t width = strlen(lbl.text) * char_w;
  s_tft->fillRect(lbl.x, lbl.y, width, char_h, UI_COLOR_BG);
}

bool hit_rect(const rect &r, int x, int y)
{
  bool val = x >= r.x && x <= (r.x + r.w) && y >= r.y && y <= (r.y + r.h);
  
  if(val){
    tone(BUZZER, BUZZER_FREQ, BUZZER_T);
  }
  
  return val;
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
  if (r.fill) {
    // 1. Draw solid filled background
    s_tft->fillRoundRect(r.x, r.y, r.w, r.h, r.radius, r.color);

    // 2. Draw darker outline around corner edges
    s_tft->drawRoundRect(r.x, r.y, r.w, r.h, r.radius, r.border_color);

    // 3. Render text over the solid fill background
    s_tft->setTextSize(r.text_size);
    s_tft->setTextColor(r.text_color, r.color); // BG matches button fill
    s_tft->setCursor(r.x + r.text_ox, r.y + r.text_oy);
    s_tft->print(r.text);
  } else {
    // 1. Draw rounded outer border line
    s_tft->drawRoundRect(r.x, r.y, r.w, r.h, r.radius, r.color);

    // 2. Render text breaking top border
    s_tft->setTextSize(r.text_size);
    s_tft->setTextColor(r.text_color, UI_COLOR_BG); // BG matches screen background
    s_tft->setCursor(r.x + r.text_ox, r.y + r.text_oy);
    s_tft->print(r.text);
  }
}

void clear_rect(const rect &r) {
  int16_t pad_top = (r.text_oy < 0) ? -r.text_oy : 0;
  s_tft->fillRect(r.x, r.y - pad_top, r.w, r.h + pad_top, UI_COLOR_BG);
}

void clear_edit_screen() {
  s_tft->fillRect(10, 6, 120, 16, UI_COLOR_BG); 

  for (uint8_t i = 0; i < EDIT_LABEL_COUNT; i++) {
    clear_label(EDIT_LABELS[i]);
  }

  for (uint8_t i = 0; i < FLD_COUNT; i++) {
    clear_rect(FIELD_RECT[i]);
  }
  for (uint8_t i = 0; i < EDIT_BTN_COUNT; i++) {
    clear_rect(EDIT_BUTTONS[i]);
  }
}

void clear_main_screen() {
  for (uint8_t i = 0; i < MAIN_LABEL_COUNT; i++) {
    clear_label(MAIN_LABELS[i]);
  }

  for (uint8_t i = 0; i < 4; i++) {
    clear_rect(SLOTS[i]);
  }
  for (uint8_t i = 0; i < 3; i++) {
    clear_rect(MAIN_BUTTONS[i]);
  }
}

void draw_label(const ui_label &lbl) {
  s_tft->setTextSize(lbl.font_size);
  s_tft->setTextColor(lbl.color, UI_COLOR_BG);
  s_tft->setCursor(lbl.x, lbl.y);
  s_tft->print(lbl.text);
}

//-------------------------------- draw MAIN SCREEN

void draw_main_static()
{ // draw static ui of the main screen
  for(uint8_t lbl = 0;lbl < MAIN_LABEL_COUNT;lbl++){
    draw_label(MAIN_LABELS[lbl]);
  }

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

    // Status indicator circle
    bool relay_on = (state == SYS_RUNNING && pr.state == RELAY_ON && !pr.finished);
    s_tft->fillCircle(r.x + r.w - 14, r.y + 16, 6, relay_on ? UI_COLOR_ACTIVE : UI_COLOR_INACTIVE);

    // Remaining time (font size 2)
    uint32_t remain = preset_time_left(pr, now, state);
    format_HMS(remain, buf, sizeof(buf));
    s_tft->setCursor(r.x + 6, r.y + 40);
    s_tft->setTextColor(UI_COLOR_FG, UI_COLOR_BG);
    s_tft->setTextSize(2);
    s_tft->print(buf);

    // Preset status (font size 2)
    s_tft->setCursor(r.x + 6, r.y + 68);
    s_tft->setTextColor(UI_COLOR_ACCENT, UI_COLOR_BG);
    s_tft->setTextSize(2);

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

void draw_edit_header(uint8_t idx) {
  s_tft->setTextSize(2);
  s_tft->setTextColor(UI_COLOR_FG, UI_COLOR_BG); // overwrite directly
  s_tft->setCursor(10, 6);
  s_tft->print("Preset ");
  s_tft->print(idx + 1);
}

void draw_edit_static(uint8_t idx)
{ // draw static ui of the edit screen
  draw_edit_header(idx);
  
  for(uint8_t lbl = 0;lbl < EDIT_LABEL_COUNT;lbl++){
    draw_label(EDIT_LABELS[lbl]);
  }

  for (uint8_t f = 0; f < FLD_COUNT; f++)
  { // draw field buttons
    draw_rect(FIELD_RECT[f]);
  }

  for (uint8_t btn = 0; btn < EDIT_BTN_COUNT; btn++)
  { // draw control buttons
    draw_rect(EDIT_BUTTONS[btn]);
  }
}

void draw_edit_dynamic(timer_preset &pr)
{
  char buf[7];

  s_tft->setTextSize(2);
  s_tft->setTextColor(UI_COLOR_FG, UI_COLOR_BG);

  for (uint8_t f = 0; f < FLD_COUNT; f++)
  {
    const rect &r = FIELD_RECT[f];
    edit_field field = (edit_field)f;

    if (selected_field != last_selected)
    {
      uint16_t border_color = (field == selected_field) ? UI_COLOR_ACCENT : r.color;
      s_tft->drawRoundRect(r.x, r.y, r.w, r.h, r.radius, border_color);
    }

    switch (f)
    {
    case FLD_TON_H:  snprintf(buf, sizeof(buf), "%03u", pr.ton.h); break;
    case FLD_TON_M:  snprintf(buf, sizeof(buf), "%02u", pr.ton.m); break;
    case FLD_TON_S:  snprintf(buf, sizeof(buf), "%02u", pr.ton.s); break;
    case FLD_TOFF_H: snprintf(buf, sizeof(buf), "%03u", pr.toff.h); break;
    case FLD_TOFF_M: snprintf(buf, sizeof(buf), "%02u", pr.toff.m); break;
    case FLD_TOFF_S: snprintf(buf, sizeof(buf), "%02u", pr.toff.s); break;
    case FLD_CYCLES: snprintf(buf, sizeof(buf), "%5u", pr.cycles); break;
    default: buf[0] = '\0'; break;
    }

    // Centered vertically inside the 42px tall box
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
        return true;
      }
    }

    // 2. check preset slots
    for (uint8_t slot = 0; slot < 4; slot++) 
    {
      if (hit_rect(SLOTS[slot], x, y)) 
      {
        editing_preset = SLOT_TO_PRESET[slot];
        selected_field = FLD_TON_H;
        last_selected = (edit_field)-1;
        screen_state = SCR_EDIT;
        clear_main_screen();
        draw_edit_static(editing_preset);
        draw_edit_dynamic(s_presets[editing_preset]);
        return true;
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
        return true;
      }
    }

    // 2. check edit control buttons (+, -, BACK, MAIN, NEXT)
    for (uint8_t i = 0; i < EDIT_BTN_COUNT; i++) 
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
            draw_edit_header(editing_preset);
            draw_edit_dynamic(s_presets[editing_preset]);
            break;

          case BTN_NEXT_IDX:
            editing_preset = (uint8_t)((editing_preset + 1) % 4);
            selected_field = FLD_TON_H;
            draw_edit_header(editing_preset);
            draw_edit_dynamic(s_presets[editing_preset]);
            break;

          case BTN_MAIN_IDX:
            screen_state = SCR_MAIN;
            clear_edit_screen();
            draw_main_static();
            draw_main_dynamic(state);
            break;
        }
        return true;
      }
    }
  }

  return false;
}

//--------------------------------