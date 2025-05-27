#pragma once

#include "assets.h"
#include <Arduino.h>
#include <EventTask.h>
#include <Game.h>
#include <Locator.h>

const uint8_t MOVE_PACE = 2;
const uint8_t MAX_MOVE_HEIGHT = 4;

class Block : public Sprite, public EventTask {
private:
  enum State { IDLE, HIT };

  Direction direction;

  bool _timeUpdated;
  String _text;

  unsigned long lastMillis = 0;
  State _state = IDLE;
  State _lastState = IDLE;
  uint8_t _lastY;
  uint8_t _firstY;

  void idle();
  void hit();
  void setTextBlock(bool forceUpdate=false);

public:
  Block(int x, int y);
  void setText(String text);
  void init();
  void update();
  const char *name();
  void execute(EventType event, Sprite *caller);
};
