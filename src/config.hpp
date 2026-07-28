#pragma once
#include <Arduino.h>

//-------------------------------- PINS

// display pins
#define TFT_CS 10
#define TFT_RST 9
#define TFT_DC 8
#define T_CS 7

// relay pins
static const uint8_t RELAY_PINS[4] = {2, 3, 4, 5};

// other pins
#define BUZZER 6

//-------------------------------- SETTINGS

// display settings
#define SCREEN_W 240
#define SCREEN_H 320
#define DYNAMIC_REFRESH_TIME 10

// edit limits
#define MAX_H 999
#define MAX_M 59
#define MAX_S 59
#define MAX_CYCLES 50000

#define BUZZER_FREQ 500
#define BUZZER_T 50

//-------------------------------- STATES

enum system_state : uint8_t
{
    SYS_IDLE = 0,
    SYS_RUNNING = 1,
    SYS_FROZEN = 2
};

//--------------------------------