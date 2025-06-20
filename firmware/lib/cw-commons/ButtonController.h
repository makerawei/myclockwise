#pragma once
#include <Arduino.h>
#include <OneButton.h>

#define PIN_INPUT 36

struct ButtonController {

  OneButton _button0;

  ButtonController() {
    _button0.setClickMs(100);
    _button0.setPressMs(500);
    _button0.setup(PIN_INPUT, INPUT_PULLUP, true);
  }
  
  static ButtonController *getInstance() {
    static ButtonController base;
    return &base;
  }

  void begin(callbackFunction clickFunc, callbackFunction longPressStartFunc, callbackFunction longPressStopFunc) {
    if(clickFunc) {
      _button0.attachClick(clickFunc);
    }
    if(longPressStartFunc) {
      _button0.attachLongPressStart(longPressStartFunc);
    }
    if(longPressStopFunc) {
      _button0.attachLongPressStop(longPressStopFunc);
    }
  }

  void loop() {
    _button0.tick();
  }
};

