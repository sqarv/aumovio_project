#include "timer_channel.hpp"

uint32_t hmsToMillis(const TimeHMS &t) {
  return ((uint32_t)t.h * 3600UL + (uint32_t)t.m * 60UL + (uint32_t)t.s) * 1000UL;
}

void setRelayPhysical(const TimerChannel &ch, bool relayOn) {
  // Modul de relee activ pe LOW: TON -> pin LOW, TOFF -> pin HIGH
  digitalWrite(ch.relayPin, relayOn ? LOW : HIGH);
}

void channelArm(TimerChannel &ch) {
  ch.phase = PH_ON;
  ch.phaseStartMs = millis();
  ch.phaseDurationMs = hmsToMillis(ch.ton);
  ch.cyclesDone = 0;
  ch.finished = false;
  setRelayPhysical(ch, true);
}

void channelStopReset(TimerChannel &ch) {
  ch.phase = PH_ON;   // stare de repaus - reincepe cu TON la urmatorul START
  ch.cyclesDone = 0;
  ch.finished = false;
  setRelayPhysical(ch, false);
}

void channelUpdate(TimerChannel &ch, SystemState state, uint32_t now) {
  if (state != SYS_RUNNING || ch.finished) return; // FROZEN/IDLE: nu avansam

  uint32_t elapsed = now - ch.phaseStartMs;
  if (elapsed < ch.phaseDurationMs) return; // inca in faza curenta

  if (ch.phase == PH_ON) {
    // TON incheiat -> trecem pe TOFF
    ch.phase = PH_OFF;
    ch.phaseDurationMs = hmsToMillis(ch.toff);
    ch.phaseStartMs = now;
    setRelayPhysical(ch, false);
  } else {
    // TOFF incheiat -> un ciclu complet [TON -> TOFF]
    ch.cyclesDone++;
    if (ch.cycles != 0 && ch.cyclesDone >= ch.cycles) {
      ch.finished = true;
      setRelayPhysical(ch, false);
      return;
    }
    ch.phase = PH_ON;
    ch.phaseDurationMs = hmsToMillis(ch.ton);
    ch.phaseStartMs = now;
    setRelayPhysical(ch, true);
  }
}

void channelShiftAfterFreeze(TimerChannel &ch, uint32_t pausedDurationMs) {
  // Mutam reperul de start cu exact durata cat a stat inghetat sistemul,
  // ca timpul ramas sa fie identic cu cel de dinainte de FREEZE.
  ch.phaseStartMs += pausedDurationMs;
}

uint32_t channelRemainingMs(const TimerChannel &ch, uint32_t effectiveNow, SystemState state) {
  if (state == SYS_IDLE) return hmsToMillis(ch.ton); // preview: durata TON completa
  if (ch.finished) return 0;
  uint32_t elapsed = effectiveNow - ch.phaseStartMs;
  if (elapsed >= ch.phaseDurationMs) return 0;
  return ch.phaseDurationMs - elapsed;
}

static uint8_t wrapAdd(uint8_t val, int8_t delta, uint8_t maxVal) {
  int16_t v = (int16_t)val + delta;
  if (v < 0) v = (int16_t)maxVal;
  if (v > (int16_t)maxVal) v = 0;
  return (uint8_t)v;
}

static uint16_t clampCycles(int32_t v) {
  if (v < 0) return 0;
  if (v > MAX_CYCLES) return MAX_CYCLES;
  return (uint16_t)v;
}

void channelAdjustField(TimerChannel &ch, EditField field, int8_t delta) {
  switch (field) {
    case FLD_TON_H:  ch.ton.h  = wrapAdd(ch.ton.h,  delta, MAX_H); break;
    case FLD_TON_M:  ch.ton.m  = wrapAdd(ch.ton.m,  delta, MAX_M); break;
    case FLD_TON_S:  ch.ton.s  = wrapAdd(ch.ton.s,  delta, MAX_S); break;
    case FLD_TOFF_H: ch.toff.h = wrapAdd(ch.toff.h, delta, MAX_H); break;
    case FLD_TOFF_M: ch.toff.m = wrapAdd(ch.toff.m, delta, MAX_M); break;
    case FLD_TOFF_S: ch.toff.s = wrapAdd(ch.toff.s, delta, MAX_S); break;
    case FLD_CYCLES: ch.cycles = clampCycles((int32_t)ch.cycles + delta); break;
    default: break;
  }
}