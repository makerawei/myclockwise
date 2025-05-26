#include "hero.h"

Hero::Hero(int x, int y) {
  _x = x;
  _y = y;
}

void Hero::move(Direction dir, int times) {
  if (dir == RIGHT) {
    _x += MARIO_PACE;
  } else if (dir == LEFT) {
    _x -= MARIO_PACE;
  }
}

bool Hero::jump() {
  bool canJump = _state != JUMPING && (millis() - lastMillis > 500);
  if (canJump) {
    _lastState = _state;
    _state = JUMPING;
    Locator::getDisplay()->fillRect(_x, _y, _width, _height, SKY_COLOR);
    _width = HERO1_SIZE[0];
    _height = HERO1_SIZE[1];
    _sprite = HERO1;

    direction = UP;

    _lastY = _y;
    _lastX = _x;
  }

  return canJump;
}

void Hero::idle() {
  if (_state != IDLE) {
    _lastState = _state;
    _state = IDLE;
    Locator::getDisplay()->fillRect(_x, _y, _width, _height, SKY_COLOR);
    _width = HERO1_SIZE[0];
    _height = HERO1_SIZE[1];
    _sprite = HERO1;
  }
}

void Hero::init() {
  Locator::getEventBus()->subscribe(this);
  Locator::getDisplay()->drawRGBBitmap(_x, _y, HERO1, HERO1_SIZE[0], HERO1_SIZE[1]);
}

void Hero::update() {
  if (_state == IDLE && _state != _lastState) {
    Locator::getDisplay()->drawRGBBitmap(_x, _y, HERO1, HERO1_SIZE[0], HERO1_SIZE[1]);
  } else if (_state == JUMPING) {
    if (millis() - lastMillis >= 50) {
      lastMillis = millis();
    }
  }
}

void Hero::execute(EventType event, Sprite *caller) {
  if (event == EventType::COLLISION) {
    direction = DOWN;
  }
}

const char *Hero::name() {  return "MARIO";
}

