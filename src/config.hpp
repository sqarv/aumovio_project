#pragma once
#include <Arduino.h>

// display pins
#define TFT_CS     10
#define TFT_RST     9    
#define TFT_DC      8
#define T_CS        7

// relay pins
static const uint8_t RELAY_PINS[4] = {2, 3, 4, 5};

// display settings
#define SCREEN_W 240
#define SCREEN_H 320

// edit limits
#define MAX_H 99
#define MAX_M 59
#define MAX_S 59
#define MAX_CYCLES 999

// systems states
enum SystemState : uint8_t { SYS_IDLE = 0, SYS_RUNNING = 1, SYS_FROZEN = 2 };