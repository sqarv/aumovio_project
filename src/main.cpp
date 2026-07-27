#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <XPT2046_Touchscreen.h>
#include <string.h>

#include "theme.hpp"
#include "config.hpp"
#include "timer_preset.hpp"
#include "ui_manager.hpp"

//-------------------------------- INIT

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST); // display object
XPT2046_Touchscreen ts(T_CS);                                   // touch object

timer_preset presets[4] = {
    {"Custom 1", RELAY_PINS[0], {0, 0, 5}, {0, 0, 5}, 3, RELAY_OFF, 0, 0, 0, false},
    {"Custom 2", RELAY_PINS[1], {0, 0, 5}, {0, 0, 5}, 3, RELAY_OFF, 0, 0, 0, false},
    {"Custom 3", RELAY_PINS[2], {0, 0, 5}, {0, 0, 5}, 3, RELAY_OFF, 0, 0, 0, false},
    {"Custom 4", RELAY_PINS[3], {0, 0, 5}, {0, 0, 5}, 3, RELAY_OFF, 0, 0, 0, false},
};

system_state current_state = SYS_IDLE;

//-------------------------------- HELPER FUNCTIONS

bool readTap(int &x, int &y)
{
  if (!ts.touched())
    return false;
  TS_Point p = ts.getPoint();

  String text = String("X: ") + String(p.x) + String(" Y: ") + String(p.y) + String(" Preassure: ") + String(p.z);
  Serial.println(text); // touch debbuging

  // map touch location to screen size
  x = map(p.x, 385, 3752, 0, SCREEN_W);
  y = map(p.y, 280, 3700, 0, SCREEN_H);

  // limit touch coordinates to screen boundaries
  x = constrain(x, 0, 240);
  y = constrain(y, 0, 320);

  return true;
}

//-------------------------------- SETUP

void setup()
{
  Serial.begin(9600);

  // set relays to off
  for (uint8_t i = 0; i < 4; i++)
  {
    pinMode(presets[i].pin, OUTPUT);
    digitalWrite(presets[i].pin, LOW);
  }

  // init display
  tft.init(SCREEN_W, SCREEN_H);
  tft.invertDisplay(0); // non-inverted colors
  tft.setRotation(0);   // portrait orientationn
  tft.fillScreen(UI_COLOR_BG);

  // init touch
  ts.begin();
  ts.setRotation(3);

  // other
  ui_init(&tft, presets);
  ui_draw_main(current_state);

  // set buzzer
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
}

//-------------------------------- LOOP

void loop()
{
  uint32_t now = millis();

  // update presets
  for (uint8_t i = 0; i < 4; i++)
  {
    preset_update(presets[i], current_state, now);
  }

  // touch detection
  int x, y;
  if (readTap(x, y))
  {
    bool pressed = ui_handle_tap(x, y, current_state);
    if(pressed){
      tone(BUZZER, BUZZER_FREQ, BUZZER_T);
    }
    delay(50);
  }

  ui_tick(current_state);
}

//--------------------------------