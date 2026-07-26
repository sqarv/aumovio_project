#pragma once
#include <Arduino.h>
#include "config.hpp"

struct TimeHMS {
  uint8_t h;
  uint8_t m;
  uint8_t s;
};

enum RelayFsmState : uint8_t { PH_ON = 0, PH_OFF = 1 };

// Campurile editabile din ecranul EDIT (folosit si de UIManager)
enum EditField : uint8_t {
  FLD_TON_H = 0, FLD_TON_M, FLD_TON_S,
  FLD_TOFF_H, FLD_TOFF_M, FLD_TOFF_S,
  FLD_CYCLES,
  FLD_COUNT
};

struct TimerChannel {
  const char* name;
  uint8_t relayPin;

  // --- Configurabil (editat din ecranul EDIT) ---
  TimeHMS ton;
  TimeHMS toff;
  uint16_t cycles;        // 0 = repetare infinita

  // --- Stare runtime FSM (nu se editeaza direct) ---
  RelayFsmState phase;
  uint32_t phaseStartMs;
  uint32_t phaseDurationMs;
  uint16_t cyclesDone;
  bool finished;          // true cand cyclesDone == cycles (si cycles != 0)
};

uint32_t hmsToMillis(const TimeHMS &t);
void setRelayPhysical(const TimerChannel &ch, bool relayOn);

void channelArm(TimerChannel &ch);                 // START: (re)porneste canalul din TON
void channelStopReset(TimerChannel &ch);            // STOP: opreste + reseteaza releul pe OFF
void channelUpdate(TimerChannel &ch, SystemState state, uint32_t now); // apelat in fiecare loop()
void channelShiftAfterFreeze(TimerChannel &ch, uint32_t pausedDurationMs); // la iesirea din FREEZE
uint32_t channelRemainingMs(const TimerChannel &ch, uint32_t effectiveNow, SystemState state);
void channelAdjustField(TimerChannel &ch, EditField field, int8_t delta); // pt butoanele +/-