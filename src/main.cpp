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
    {"Custom 1", RELAY_PINS[0], {0, 0, 0}, {0, 0, 0}, 0, RELAY_OFF, 0, 0, 0, false},
    {"Custom 2", RELAY_PINS[1], {0, 0, 0}, {0, 0, 0}, 0, RELAY_OFF, 0, 0, 0, false},
    {"Custom 3", RELAY_PINS[2], {0, 0, 0}, {0, 0, 0}, 0, RELAY_OFF, 0, 0, 0, false},
    {"Custom 4", RELAY_PINS[3], {0, 0, 0}, {0, 0, 0}, 0, RELAY_OFF, 0, 0, 0, false},
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

// Timing parameters
const uint16_t BASE_SPEED = 250;  // Initial step delay (ms)
const uint16_t MIN_SPEED  = 10;   // Fastest step delay when held (ms)
const uint16_t ACCEL_STEP = 10;   // How fast it accelerates per tick (ms)

uint32_t last_action_time = 0;
uint16_t current_delay = BASE_SPEED;
bool touch_state = false;

void handle_touch(int x, int y, uint32_t now) {
  bool touching = readTap(x, y);

  if (touching) {
    // 1. First touch: Trigger immediately and reset acceleration
    if (!touch_state) {
      touch_state = true;
      current_delay = BASE_SPEED;
      last_action_time = now;
      ui_handle_tap(x, y, current_state);
    } 
    // 2. Held down: Trigger repeatedly with decreasing delay
    else if (now - last_action_time >= current_delay) {
      last_action_time = now;
      ui_handle_tap(x, y, current_state);

      // Accelerate (decrease delay down to MIN_SPEED)
      if (current_delay > MIN_SPEED + ACCEL_STEP) {
        current_delay -= ACCEL_STEP;
      } else {
        current_delay = MIN_SPEED;
      }
    }
  } else {
    // Touch released: Reset state
    touch_state = false;
  }
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
  
  int x,y;
  handle_touch(x,y,now);
  
  // update ui
  ui_tick(current_state);
}

//--------------------------------