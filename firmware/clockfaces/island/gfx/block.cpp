#include "block.h"
#include "assets.h"

// String &Block::_text;

Block::Block(int x, int y) {
  _x = x;
  _y = y;
  _firstY = y;
  _width = 39;
  _height = 12;
}

void Block::idle() {
  if (_state != IDLE) {
    // Serial.println("Block - Idle - Start");

    _lastState = _state;
    _state = IDLE;

    _y = _firstY;
  }
}

void Block::hit() {
  if (_state != HIT) {
    // Serial.println("Hit - Start");

    _lastState = _state;
    _state = HIT;

    _lastY = _y;

    direction = UP;
  }
}

void Block::setTextBlock(bool forceUpdate) {
  Serial.println("setTextBlock");
  if(_timeUpdated || forceUpdate) {
    _timeUpdated = false;
    const char *timeStr = _text.c_str();
    for(uint8_t i = 0; i < 4; i++) {
      const uint8_t offset = i >= 2 ? 3 : 0;
      Locator::getDisplay()->drawRGBBitmap(1 + _x + i * NUMBER_WIDTH + offset, _y + 2, NUMBERS[timeStr[i] - '0'], NUMBER_WIDTH, NUMBER_HEIGHT);
    }
  }
}

void Block::setText(String text) {
  Serial.printf("setText, text=%s, _text=%s\r\n", text.c_str(), _text.c_str());
  if(strcmp(_text.c_str(), text.c_str()) != 0) {
    _text = text;
    _timeUpdated = true;
  }
}

void Block::init() {
  Locator::getEventBus()->subscribe(this);
  Locator::getDisplay()->drawRGBBitmap(_x, _y, BLOCK, _width, _height);
  setTextBlock();
}

void Block::update() {

  if (_state == IDLE && _lastState != _state) {
    Locator::getDisplay()->drawRGBBitmap(_x, _y, BLOCK, _width, _height);
    setTextBlock(true);
    _lastState = _state;

  } else if (_state == HIT) {

    if (millis() - lastMillis >= 60) {

      // Serial.print("BLOCK Y = ");
      // Serial.println(_y);

      Locator::getDisplay()->fillRect(_x, _y, _width, _height, SKY_COLOR);

      _y = _y + (MOVE_PACE * (direction == UP ? -1 : 1));

      Locator::getDisplay()->drawRGBBitmap(_x, _y, BLOCK, _width, _height);
      setTextBlock(true);

      if (floor(_firstY - _y) >= MAX_MOVE_HEIGHT) {
        // Serial.println("DOWN");
        direction = DOWN;
      }

      if (_y >= _firstY && direction == DOWN) {
        idle();
      }

      lastMillis = millis();
    }
  }
}

void Block::execute(EventType event, Sprite *caller) {
  // Serial.printf("block Checking collision by eventType %d\n", event);
  if (event == EventType::MOVE) {
    if (this->collidedWith(caller)) {
      Serial.println("Collision detected");
      hit();
      Locator::getEventBus()->broadcast(EventType::COLLISION, this);
    }
  } else if (event == EventType::COLLISION) {
    hit();
  }
}

const char *Block::name() { return "BLOCK"; }