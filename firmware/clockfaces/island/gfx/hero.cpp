#include "hero.h"

const uint16_t *HERO_FRAME_PTRS[] = {HERO1, HERO2, HERO3};
const uint8_t HERO_FRAME_COUNT = sizeof(HERO_FRAME_PTRS) / sizeof(HERO_FRAME_PTRS[0]);

Hero::Hero(int x, int y) {
  _x = x;
  _y = y;
  _currentFrame = 0;
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
  }
}

void Hero::init() {
  Locator::getEventBus()->subscribe(this);
  Locator::getDisplay()->drawRGBBitmap(_x, _y, HERO1, HERO1_SIZE[0], HERO1_SIZE[1]);
}

void Hero::update() {
  if (millis() - lastMillis < 100) {
    return;
  }
  lastMillis = millis();
  const uint16_t *hero = (const uint16_t*)pgm_read_ptr(&HERO_FRAME_COUNT[_currentFrame]);
  Locator::getDisplay()->drawRGBBitmap(_x, _y, hero, HERO1_SIZE[0], HERO1_SIZE[1]);
  if(++_currentFrame >= HERO_FRAME_COUNT) {
    _currentFrame = 0;
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
