#include "hero.h"

const uint16_t *HERO_FRAME_PTRS[] = {HERO1, HERO2, HERO3};
const uint8_t HERO_FRAME_COUNT = sizeof(HERO_FRAME_PTRS) / sizeof(HERO_FRAME_PTRS[0]);

Hero::Hero(int x, int y) {
  _x = x;
  _y = y;
}

bool Hero::jump() {
  bool canJump = _state != JUMPING;
  if(!canJump) {
    return false;
  }
  
  Locator::getDisplay()->fillRect(_x, _y, _width, _height, SKY_COLOR);
  _lastState = _state;
  _duration = 120;
  _state = JUMPING;
  direction = UP;
  _lastY = _y;
  _lastX = _x;
  _width = HERO_JUMP_SIZE[0];
  _height = HERO_JUMP_SIZE[1];
  
  return true;
}

void Hero::idle() {
  if (_state != IDLE) {
    Locator::getDisplay()->fillRect(_x, _y, _width, _height, SKY_COLOR);
    _lastState = _state;
    _duration = 100;
    _state = IDLE;
    _width = HERO1_SIZE[0];
    _height = HERO1_SIZE[1];
  }
}

void Hero::init() {
  Locator::getEventBus()->subscribe(this);
  Locator::getDisplay()->drawRGBBitmap(_x, _y, HERO1, HERO1_SIZE[0], HERO1_SIZE[1]);
}

void Hero::update() {
  if (millis() - lastMillis < _duration) {
    return;
  }
  lastMillis = millis();
  if(_state == IDLE) {
    const uint16_t *hero = (const uint16_t*)pgm_read_ptr(&HERO_FRAME_PTRS[_currentFrame]);
    Locator::getDisplay()->drawRGBBitmap(_x, _y, hero, HERO1_SIZE[0], HERO1_SIZE[1]);
    if(++_currentFrame >= HERO_FRAME_COUNT) {
      _currentFrame = 0;
    }
  } else if(_state == JUMPING) {
    Locator::getDisplay()->fillRect(_x, _y, _width, _height, SKY_COLOR);
    _y = _y + (HERO_PACE * (direction == UP ? -1 : 1));
    Locator::getDisplay()->drawRGBBitmap(_x, _y, HERO_JUMP, _width, _height);
    Locator::getEventBus()->broadcast(MOVE, this);
    if (floor(_lastY - _y) >= HERO_JUMP_HEIGHT) {
      direction = DOWN;
    }    
    if (_y + _height >= 48) {
      idle();
    }
  }
}

void Hero::execute(EventType event, Sprite *caller) {
  if (event == EventType::COLLISION) {
    direction = DOWN;
  }
}

const char *Hero::name() {
  return "GaoQiaoMingRen";
}
