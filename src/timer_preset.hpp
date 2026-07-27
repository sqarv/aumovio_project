#pragma once
#include <Arduino.h>
#include "config.hpp"

//-------------------------------- DATA STRUCTURES DEFINITIONS

struct time_HMS
{
  uint16_t h;
  uint8_t m;
  uint8_t s;
};

enum relay_fsm_state : uint8_t
{
  RELAY_ON = 0,
  RELAY_OFF = 1
};

enum edit_field : uint8_t
{
  FLD_TON_H = 0,
  FLD_TON_M,
  FLD_TON_S,
  FLD_TOFF_H,
  FLD_TOFF_M,
  FLD_TOFF_S,
  FLD_CYCLES,
  FLD_COUNT
};

struct timer_preset
{
  const char *name;
  uint8_t pin;

  // configurable from the edit screen
  time_HMS ton;
  time_HMS toff;
  uint16_t cycles;

  // states
  relay_fsm_state state;
  uint32_t state_start_ms;
  uint32_t state_duration_ms;
  uint16_t cycles_done;
  bool finished; // true when cyclesDone == cycles (and cycles != 0)
};

//-------------------------------- FUNCTION DECLARATIONS

uint32_t HMS_to_millis(const time_HMS &t);
void set_relay(const timer_preset &pr, bool state);

void preset_on(timer_preset &pr);
void preset_off(timer_preset &pr);
void preset_update(timer_preset &pr, system_state state, uint32_t now);  // update preset timer
void preset_shift_freeze(timer_preset &pr, uint32_t paused_duration_ms); // shift state_start time by freeze duration
uint32_t preset_time_left(const timer_preset &pr, uint32_t now, system_state state);
void adjust_preset_field(timer_preset &pr, edit_field field, int8_t delta);

//--------------------------------