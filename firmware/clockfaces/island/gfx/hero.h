#pragma once

#include "assets.h"
#include <stdbool.h>
#include <Arduino.h>
#include <EventBus.h>
#include <Game.h>
#include <ImageUtils.h>
#include <Locator.h>

const uint8_t HERO_PACE = 3;
const uint8_t HERO_JUMP_HEIGHT = 14;

class Hero : public Sprite, public EventTask {
private:
  enum State { IDLE, WALKING, JUMPING };

  Direction direction;

  int _lastX;
  int _lastY;

  uint8_t _currentFrame = 0;
  unsigned long lastMillis = 0;
  uint16_t _duration = 100;
  State _state = IDLE;
  State _lastState = IDLE;

  void idle();

public:
  Hero(int x, int y);
  void init();
  bool jump();
  void update();
  const char *name();
  void execute(EventType event, Sprite *caller);
};
