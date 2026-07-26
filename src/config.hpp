#pragma once
#include <Arduino.h>

// ---------- Pini display (SPI) ----------
#define TFT_CS 10
#define TFT_DC   9
#define TFT_RST  8
#define T_CS 7

// ---------- Pini relee (patru canale) ----------
// Modulul de relee e activ pe LOW (comun la modulele cu optocuplor):
//   TON  -> pin LOW  -> releu ON
//   TOFF -> pin HIGH -> releu OFF
static const uint8_t RELAY_PIN[4] = {2, 3, 4, 5};

// ---------- Pin trigger extern (stand-in camera climatica) ----------
// Declarat, dar neintegrat inca in FSM (ramane pt o etapa urmatoare).
#define TRIGGER_PIN 6

// ---------- Ecran ----------
#define SCREEN_W 240
#define SCREEN_H 320

// ---------- Limite editare ----------
#define MAX_H 99
#define MAX_M 59
#define MAX_S 59
#define MAX_CYCLES 999   // Cicluri = 0 => repetare infinita

// ---------- Stare globala sistem (comenzi master) ----------
enum SystemState : uint8_t { SYS_IDLE = 0, SYS_RUNNING = 1, SYS_FROZEN = 2 };