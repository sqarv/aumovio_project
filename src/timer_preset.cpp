#include "timer_preset.hpp"

//-------------------------------- HELPER FUNCTIONS

static uint8_t wrap_add(uint8_t val, int8_t delta, uint8_t maxVal) { // used for H , M ... values . ex:  99 <- 0 ... 99 -> 0
  int16_t v = (int16_t)val + delta;
  if (v < 0) v = (int16_t)maxVal;
  if (v > (int16_t)maxVal) v = 0;
  return (uint8_t)v;
}

//-------------------------------- FUNCTION DEFINITIONS

uint32_t HMS_to_millis(const time_HMS &t) {
  return ((uint32_t)t.h * 3600UL + (uint32_t)t.m * 60UL + (uint32_t)t.s) * 1000UL;
}

void set_relay(const timer_preset &pr, bool state) {
  digitalWrite(pr.pin,state);
}

void preset_on(timer_preset &pr) {
  pr.state = RELAY_ON;
  pr.state_start_ms = millis();
  pr.state_duration_ms = HMS_to_millis(pr.ton);
  pr.cycles_done = 0;
  pr.finished = false;
  set_relay(pr, true);
}

void preset_off(timer_preset &pr) {
  pr.state = RELAY_OFF;   // stare de repaus - reincepe cu TON la urmatorul START
  pr.cycles_done = 0;
  pr.finished = false;
  set_relay(pr, false);
}

void preset_update(timer_preset &pr, system_state state, uint32_t now) {
  if (state != SYS_RUNNING || pr.finished) return; // relay in FROZEN/IDLE state -> skip

  uint32_t elapsed = now - pr.state_start_ms; // elapsed time
  if (elapsed < pr.state_duration_ms) return; // relay in the same state

  // current relay state ended logic
  
  if (pr.state == RELAY_ON) 
  { // RELAY_ON state (TON) ended 
    pr.state = RELAY_OFF;
    pr.state_duration_ms = HMS_to_millis(pr.toff);
    pr.state_start_ms = now;
    set_relay(pr, false);
  } 
  else 
  { // RELAY_OFF state (TOFF) ended -> 1 cycle finished
    pr.cycles_done++;
    
    if (pr.cycles != 0 && pr.cycles_done >= pr.cycles) { // stop if set number of cycles has been reached
      pr.finished = true;
      set_relay(pr, false);
      return;
    }
    
    // cycles_done < cycles -> continue
    pr.state = RELAY_ON;
    pr.state_duration_ms = HMS_to_millis(pr.ton);
    pr.state_start_ms = now;
    set_relay(pr, true);
  }
  
}

void preset_shift_freeze(timer_preset &pr, uint32_t paused_duration_ms){
  pr.state_start_ms += paused_duration_ms;
}

uint32_t preset_time_left(const timer_preset &pr, uint32_t now, system_state state) {
  if (state == SYS_IDLE) return HMS_to_millis(pr.ton); // preview: TON
  if (pr.finished) return 0;
  
  uint32_t elapsed = now - pr.state_start_ms;
  if (elapsed >= pr.state_duration_ms) return 0;
  return pr.state_duration_ms - elapsed;
}

void adjust_preset_field(timer_preset &pr, edit_field field, int8_t delta){
  switch (field) {
    case FLD_TON_H:  pr.ton.h  = wrap_add(pr.ton.h,  delta, MAX_H); break;
    case FLD_TON_M:  pr.ton.m  = wrap_add(pr.ton.m,  delta, MAX_M); break;
    case FLD_TON_S:  pr.ton.s  = wrap_add(pr.ton.s,  delta, MAX_S); break;
    case FLD_TOFF_H: pr.toff.h = wrap_add(pr.toff.h, delta, MAX_H); break;
    case FLD_TOFF_M: pr.toff.m = wrap_add(pr.toff.m, delta, MAX_M); break;
    case FLD_TOFF_S: pr.toff.s = wrap_add(pr.toff.s, delta, MAX_S); break;
    case FLD_CYCLES: pr.cycles = wrap_add(pr.cycles, delta, MAX_CYCLES); break;
    default: break;
  }
}

//--------------------------------