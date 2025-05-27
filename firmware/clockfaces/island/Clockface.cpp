#include "Clockface.h"

const char* FORMAT_TWO_DIGITS = "%02d";

Tile ground(GROUND, 16, 13); // 地面
Object statue(STATUE, 23, 35); // 神像
Object cloud1(CLOUD1, 12, 11); // 云朵1
Object cloud2(CLOUD2, 10, 9); // 云朵2
Object roadSign(ROAD_SIGN, 12, 16); //路标

Hero hero(24, 19); // 高桥名人
Block timeBlock(13, 3); // 时间显示砖块

unsigned long lastMillis = 0;

Clockface::Clockface(Adafruit_GFX* display) : IClockface(display) {
  IClockface::alarmSetTickFunc(&Clockface::alarmTickCallback);
  IClockface::alarmSetSoundUrl(SOUND_ALARM_CLOCK_URL);
}

void Clockface::alarmTickCallback() {
  Locator::getEventBus()->broadcast(COLLISION, &hero);
}

void Clockface::setup() {
  Locator::getDisplay()->fillRect(0, 0, 64, 64, SKY_COLOR);

  ground.fillRow(DISPLAY_HEIGHT - ground._height);

  statue.draw(0, 16);
  roadSign.draw(52, 35);
  cloud1.draw(0, 1);
  cloud2.draw(54, 12);

  updateTime();

  timeBlock.init();
  hero.init();
}

void Clockface::update() {
  timeBlock.update();
  hero.update();
  if (_dateTime->getSecond() == 0 && millis() - lastMillis >= 1000) {
    if(!isAlarmTaskRunning()) {
      hero.jump();
    }
    updateTime();
    lastMillis = millis();
  }
}

void Clockface::updateTime() {
  IClockface::updateTime();
  timeBlock.setText(String(_dateTime->getHour(FORMAT_TWO_DIGITS)) + String(_dateTime->getMinute(FORMAT_TWO_DIGITS)));
}

void Clockface::jumpSoundTask(void *args) {
  String url = SOUND_BUTTON_CLICK_URL;
  while(true) {
    vTaskDelay(pdMS_TO_TICKS(200)); //等待200ms后马里奥跳跃可顶到砖块
    AudioHelper::getInstance()->play(url);
    break;
  }
  vTaskDelete(NULL);
}


bool Clockface::externalEvent(int type) {
  // 如果闹钟已经开始，按下按钮取消闹钟，不再播放跳起的音效
  if(tryToCancelAlarmTask() || isNightMode()) {
    return false;
  }
  if (type == 0) {  //TODO create an enum
    if(hero.jump()) {
      AudioHelper::play(&Clockface::jumpSoundTask, SOUND_BUTTON_CLICK_URL, 0);
    }
  }
  return true;
}
