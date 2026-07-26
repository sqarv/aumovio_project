/*
  Shadow Timer - FSM complet (4 canale independente) + GUI (MAIN/EDIT)
  ---------------------------------------------------------------------
  TimerChannel.h/.cpp = FSM-ul fiecarui canal (logica pura, fara ecran).
  UIManager.h/.cpp    = ecranele MAIN/EDIT si rutarea input-ului tactil.
  Acest fisier doar leaga totul: init hardware, bucla non-blocanta.

  Touch-ul e citit prin FT6206 (simulare Wokwi, I2C). Pe hardware real
  (XPT2046, SPI) se rescrie DOAR functia readTap() de mai jos.
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>

#include "config.hpp"
#include "timer_channel.hpp"
#include "ui_manager.hpp"

Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(T_CS);

TimerChannel channels[4] = {
  { "Custom 1", RELAY_PIN[0], {0,0,5}, {0,0,5}, 3, PH_ON, 0, 0, 0, false },
  { "Custom 2", RELAY_PIN[1], {0,0,5}, {0,0,5}, 3, PH_ON, 0, 0, 0, false },
  { "Custom 3", RELAY_PIN[2], {0,0,5}, {0,0,5}, 3, PH_ON, 0, 0, 0, false },
  { "Custom 4", RELAY_PIN[3], {0,0,5}, {0,0,5}, 3, PH_ON, 0, 0, 0, false },
};

SystemState systemState = SYS_IDLE;

bool readTap(int &x, int &y) {
  if (!ts.touched()) return false;
  TS_Point p = ts.getPoint();
  x = map(p.x, 0, 240, 240, 0);
  y = map(p.y, 0, 320, 320, 0);
  return true;
}

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < 4; i++) {
    pinMode(channels[i].relayPin, OUTPUT);
    digitalWrite(channels[i].relayPin, HIGH); // OFF (modul activ pe LOW)
  }
  pinMode(TRIGGER_PIN, INPUT_PULLUP); // TODO: integrare trigger extern (etapa urmatoare)
  
  tft.init(SCREEN_W,SCREEN_H);
  tft.setRotation(0);
  if (!ts.begin()) {
    Serial.println("FT6206 nu a pornit (relevant doar in simulare)");
  }

  uiInit(&tft, channels);
  uiDrawMain(systemState);
}

void loop() {
  uint32_t now = millis();

  for (uint8_t i = 0; i < 4; i++) {
    channelUpdate(channels[i], systemState, now);
  }

  int x, y;
  if (readTap(x, y)) {
    uiHandleTap(x, y, systemState);
    delay(180); // debounce simplu
  }

  uiTick(systemState);
}
