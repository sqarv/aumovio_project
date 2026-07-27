#pragma once
#include <Adafruit_ST7789.h>
#include "timer_preset.hpp"
#include "config.hpp"

enum Screen : uint8_t { SCR_MAIN = 0, SCR_EDIT = 1 };

void uiInit(Adafruit_ST7789 *tftPtr, TimerChannel *channelsPtr);
void uiDrawMain(SystemState state);       // desenare completa MAIN (o singura data, la boot/revenire)
void uiHandleTap(int x, int y, SystemState &state); // rutare tap catre ecranul curent
void uiTick(SystemState state);           // refresh periodic al numaratorii (doar pe MAIN)