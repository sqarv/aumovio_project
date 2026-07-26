#include "ui_manager.hpp"
#include <stdio.h>

struct Rect { int16_t x, y, w, h; };

static bool hitRect(const Rect &r, int x, int y) {
  return x >= r.x && x <= (r.x + r.w) && y >= r.y && y <= (r.y + r.h);
}

// ---------- Layout MAIN (240x320) ----------
// Ordinea vizuala a cardurilor urmeaza schita originala (Custom1, Custom3
// sus; Custom2, Custom4 jos) - de-aia mapam slot -> index canal logic.
static const Rect CARD[4] = {
  {10,  10, 100, 90},  // slot0 -> vizual "Custom 1"
  {130, 10, 100, 90},  // slot1 -> vizual "Custom 3"
  {10, 156, 100, 90},  // slot2 -> vizual "Custom 2"
  {130,156, 100, 90},  // slot3 -> vizual "Custom 4"
};
static const uint8_t SLOT_TO_CHANNEL[4] = {0, 2, 1, 3};

static const Rect BTN_START  = {8,  112, 72, 34};
static const Rect BTN_STOP   = {86, 112, 68, 34};
static const Rect BTN_FREEZE = {160,112, 72, 34};

// ---------- Layout EDIT ----------
static const Rect FIELD_RECT[FLD_COUNT] = {
  {20,  34, 60, 34}, // TON_H
  {90,  34, 60, 34}, // TON_M
  {160, 34, 60, 34}, // TON_S
  {20,  84, 60, 34}, // TOFF_H
  {90,  84, 60, 34}, // TOFF_M
  {160, 84, 60, 34}, // TOFF_S
  {70, 134, 100,34}, // CYCLES
};
static const Rect BTN_MINUS = {40, 180, 60, 40};
static const Rect BTN_PLUS  = {140,180, 60, 40};
static const Rect BTN_BACK  = {5,  270, 70, 40};
static const Rect BTN_MAIN2 = {85, 270, 70, 40};
static const Rect BTN_NEXT  = {165,270, 70, 40};

// ---------- Stare interna UI ----------
static Adafruit_ST7789 *s_tft = nullptr;
static TimerChannel *s_channels = nullptr;
static Screen s_screen = SCR_MAIN;
static uint8_t s_editingChannel = 0;
static EditField s_selectedField = FLD_TON_H;
static uint32_t s_freezeStartMs = 0;
static uint32_t s_lastDynamicMs = 0;

static void formatHMS(uint32_t ms, char *buf, size_t bufLen) {
  uint32_t totalSec = ms / 1000UL;
  uint16_t h = (uint16_t)((totalSec / 3600UL) % 100UL);
  uint8_t m = (uint8_t)((totalSec / 60UL) % 60UL);
  uint8_t s = (uint8_t)(totalSec % 60UL);
  snprintf(buf, bufLen, "%02u:%02u:%02u", h, m, s);
}

static void drawMainStatic() {
  s_tft->fillScreen(ST77XX_BLACK);
  static const char* labels[4] = {"Custom 1", "Custom 3", "Custom 2", "Custom 4"};
  s_tft->setTextSize(1);
  for (uint8_t slot = 0; slot < 4; slot++) {
    const Rect &r = CARD[slot];
    s_tft->drawRect(r.x, r.y, r.w, r.h, ST77XX_WHITE);
    s_tft->setCursor(r.x + 6, r.y + 6);
    s_tft->setTextColor(ST77XX_WHITE);
    s_tft->print(labels[slot]);
  }

  s_tft->drawRect(BTN_START.x, BTN_START.y, BTN_START.w, BTN_START.h, ST77XX_GREEN);
  s_tft->setCursor(BTN_START.x + 10, BTN_START.y + 12);
  s_tft->setTextColor(ST77XX_GREEN);
  s_tft->print("START");

  s_tft->drawRect(BTN_STOP.x, BTN_STOP.y, BTN_STOP.w, BTN_STOP.h, ST77XX_RED);
  s_tft->setCursor(BTN_STOP.x + 10, BTN_STOP.y + 12);
  s_tft->setTextColor(ST77XX_RED);
  s_tft->print("STOP");

  s_tft->drawRect(BTN_FREEZE.x, BTN_FREEZE.y, BTN_FREEZE.w, BTN_FREEZE.h, ST77XX_CYAN);
  s_tft->setCursor(BTN_FREEZE.x + 4, BTN_FREEZE.y + 12);
  s_tft->setTextColor(ST77XX_CYAN);
  s_tft->print("FREEZE");
}

static void drawMainDynamic(SystemState state) {
  uint32_t now = (state == SYS_FROZEN) ? s_freezeStartMs : millis();
  char buf[9];
  for (uint8_t slot = 0; slot < 4; slot++) {
    uint8_t chIdx = SLOT_TO_CHANNEL[slot];
    TimerChannel &ch = s_channels[chIdx];
    const Rect &r = CARD[slot];

    bool relayOn = (state == SYS_RUNNING && ch.phase == PH_ON && !ch.finished);
    s_tft->fillCircle(r.x + r.w - 14, r.y + 14, 6, relayOn ? ST77XX_GREEN : ST77XX_RED);

    s_tft->fillRect(r.x + 4, r.y + 40, r.w - 8, 14, ST77XX_BLACK);
    uint32_t remain = channelRemainingMs(ch, now, state);
    formatHMS(remain, buf, sizeof(buf));
    s_tft->setCursor(r.x + 6, r.y + 42);
    s_tft->setTextColor(ST77XX_WHITE);
    s_tft->setTextSize(1);
    s_tft->print(buf);

    s_tft->fillRect(r.x + 4, r.y + 60, r.w - 8, 12, ST77XX_BLACK);
    s_tft->setCursor(r.x + 6, r.y + 62);
    s_tft->setTextColor(ST77XX_YELLOW);
    if (state == SYS_IDLE) s_tft->print("IDLE");
    else if (state == SYS_FROZEN) s_tft->print("FREEZE");
    else if (ch.finished) s_tft->print("DONE");
    else s_tft->print(ch.phase == PH_ON ? "ON" : "OFF");
  }
}

static void drawEditStatic(uint8_t idx) {
  s_tft->fillScreen(ST77XX_BLACK);
  s_tft->setTextSize(2);
  s_tft->setTextColor(ST77XX_WHITE);
  s_tft->setCursor(10, 6);
  s_tft->print("Custom ");
  s_tft->print(idx + 1);

  s_tft->setTextSize(1);
  s_tft->setCursor(20, 24);
  s_tft->print("TON (H:M:S)");
  s_tft->setCursor(20, 74);
  s_tft->print("TOFF (H:M:S)");
  s_tft->setCursor(70, 124);
  s_tft->print("Cicluri (0=infinit)");

  for (uint8_t f = 0; f < FLD_COUNT; f++) {
    const Rect &r = FIELD_RECT[f];
    s_tft->drawRect(r.x, r.y, r.w, r.h, ST77XX_WHITE);
  }

  s_tft->drawRect(BTN_MINUS.x, BTN_MINUS.y, BTN_MINUS.w, BTN_MINUS.h, ST77XX_WHITE);
  s_tft->setTextSize(2);
  s_tft->setTextColor(ST77XX_WHITE);
  s_tft->setCursor(BTN_MINUS.x + 24, BTN_MINUS.y + 12);
  s_tft->print("-");

  s_tft->drawRect(BTN_PLUS.x, BTN_PLUS.y, BTN_PLUS.w, BTN_PLUS.h, ST77XX_WHITE);
  s_tft->setCursor(BTN_PLUS.x + 24, BTN_PLUS.y + 10);
  s_tft->print("+");

  s_tft->setTextSize(1);
  s_tft->drawRect(BTN_BACK.x, BTN_BACK.y, BTN_BACK.w, BTN_BACK.h, ST77XX_CYAN);
  s_tft->setCursor(BTN_BACK.x + 6, BTN_BACK.y + 14);
  s_tft->setTextColor(ST77XX_CYAN);
  s_tft->print("<BACK");

  s_tft->drawRect(BTN_MAIN2.x, BTN_MAIN2.y, BTN_MAIN2.w, BTN_MAIN2.h, ST77XX_YELLOW);
  s_tft->setCursor(BTN_MAIN2.x + 16, BTN_MAIN2.y + 14);
  s_tft->setTextColor(ST77XX_YELLOW);
  s_tft->print("MAIN");

  s_tft->drawRect(BTN_NEXT.x, BTN_NEXT.y, BTN_NEXT.w, BTN_NEXT.h, ST77XX_CYAN);
  s_tft->setCursor(BTN_NEXT.x + 10, BTN_NEXT.y + 14);
  s_tft->setTextColor(ST77XX_CYAN);
  s_tft->print("NEXT>");
}

static void drawEditDynamic(TimerChannel &ch) {
  char buf[6];
  for (uint8_t f = 0; f < FLD_COUNT; f++) {
    const Rect &r = FIELD_RECT[f];
    uint16_t color = ((EditField)f == s_selectedField) ? ST77XX_YELLOW : ST77XX_WHITE;
    s_tft->drawRect(r.x, r.y, r.w, r.h, color);
    s_tft->fillRect(r.x + 2, r.y + 2, r.w - 4, r.h - 4, ST77XX_BLACK);
    s_tft->setTextSize(2);
    s_tft->setTextColor(ST77XX_WHITE);
    s_tft->setCursor(r.x + 8, r.y + 8);
    switch (f) {
      case FLD_TON_H:  snprintf(buf, sizeof(buf), "%02u", ch.ton.h);  break;
      case FLD_TON_M:  snprintf(buf, sizeof(buf), "%02u", ch.ton.m);  break;
      case FLD_TON_S:  snprintf(buf, sizeof(buf), "%02u", ch.ton.s);  break;
      case FLD_TOFF_H: snprintf(buf, sizeof(buf), "%02u", ch.toff.h); break;
      case FLD_TOFF_M: snprintf(buf, sizeof(buf), "%02u", ch.toff.m); break;
      case FLD_TOFF_S: snprintf(buf, sizeof(buf), "%02u", ch.toff.s); break;
      case FLD_CYCLES: snprintf(buf, sizeof(buf), "%3u",  ch.cycles); break;
      default: buf[0] = '\0'; break;
    }
    s_tft->print(buf);
  }
}

void uiInit(Adafruit_ST7789 *tftPtr, TimerChannel *channelsPtr) {
  s_tft = tftPtr;
  s_channels = channelsPtr;
}

void uiDrawMain(SystemState state) {
  s_screen = SCR_MAIN;
  drawMainStatic();
  drawMainDynamic(state);
}

void uiTick(SystemState state) {
  if (s_screen != SCR_MAIN) return;
  uint32_t now = millis();
  if (now - s_lastDynamicMs < 500) return;
  s_lastDynamicMs = now;
  drawMainDynamic(state);
}

void uiHandleTap(int x, int y, SystemState &state) {
  if (s_screen == SCR_MAIN) {
    if (hitRect(BTN_START, x, y)) {
      if (state == SYS_IDLE) {
        for (uint8_t i = 0; i < 4; i++) channelArm(s_channels[i]);
        state = SYS_RUNNING;
      }
      drawMainDynamic(state);
      return;
    }
    if (hitRect(BTN_STOP, x, y)) {
      for (uint8_t i = 0; i < 4; i++) channelStopReset(s_channels[i]);
      state = SYS_IDLE;
      drawMainDynamic(state);
      return;
    }
    if (hitRect(BTN_FREEZE, x, y)) {
      if (state == SYS_RUNNING) {
        s_freezeStartMs = millis();
        state = SYS_FROZEN;
      } else if (state == SYS_FROZEN) {
        uint32_t paused = millis() - s_freezeStartMs;
        for (uint8_t i = 0; i < 4; i++) channelShiftAfterFreeze(s_channels[i], paused);
        state = SYS_RUNNING;
      }
      drawMainDynamic(state);
      return;
    }
    for (uint8_t slot = 0; slot < 4; slot++) {
      if (hitRect(CARD[slot], x, y)) {
        s_editingChannel = SLOT_TO_CHANNEL[slot];
        s_selectedField = FLD_TON_H;
        s_screen = SCR_EDIT;
        drawEditStatic(s_editingChannel);
        drawEditDynamic(s_channels[s_editingChannel]);
        return;
      }
    }
  } else { // SCR_EDIT
    TimerChannel &ch = s_channels[s_editingChannel];
    for (uint8_t f = 0; f < FLD_COUNT; f++) {
      if (hitRect(FIELD_RECT[f], x, y)) {
        s_selectedField = (EditField)f;
        drawEditDynamic(ch);
        return;
      }
    }
    if (hitRect(BTN_MINUS, x, y)) { channelAdjustField(ch, s_selectedField, -1); drawEditDynamic(ch); return; }
    if (hitRect(BTN_PLUS,  x, y)) { channelAdjustField(ch, s_selectedField, +1); drawEditDynamic(ch); return; }
    if (hitRect(BTN_BACK, x, y)) {
      s_editingChannel = (uint8_t)((s_editingChannel + 3) % 4);
      s_selectedField = FLD_TON_H;
      drawEditStatic(s_editingChannel);
      drawEditDynamic(s_channels[s_editingChannel]);
      return;
    }
    if (hitRect(BTN_NEXT, x, y)) {
      s_editingChannel = (uint8_t)((s_editingChannel + 1) % 4);
      s_selectedField = FLD_TON_H;
      drawEditStatic(s_editingChannel);
      drawEditDynamic(s_channels[s_editingChannel]);
      return;
    }
    if (hitRect(BTN_MAIN2, x, y)) {
      s_screen = SCR_MAIN;
      drawMainStatic();
      drawMainDynamic(state);
      return;
    }
  }
}