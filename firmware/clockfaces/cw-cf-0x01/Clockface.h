#pragma once

#include <Arduino.h>

#include "gfx/Super_Mario_Bros__24pt7b.h"

#include <Adafruit_GFX.h>
#include <Tile.h>
#include <Locator.h>
#include <Game.h>
#include <Object.h>
#include <ImageUtils.h>
// Commons
#include <IClockface.h>
#include <CWDateTime.h>
#include <AudioHelper.h>

#include "gfx/assets.h"
#include "gfx/mario.h"
#include "gfx/block.h"

#define SOUND_ALARM_CLOCK_URL "https://makerawei-1251006064.cos.ap-guangzhou.myqcloud.com/clockwise/mario_start.wav"
#define SOUND_BUTTON_CLICK_URL "http://makerawei-1251006064.cos.ap-guangzhou.myqcloud.com/clockwise/mario_icon.wav"

class Clockface: public IClockface {
  private:
    void updateTime();

  public:
    Clockface(Adafruit_GFX* display);
    void setup(CWDateTime *dateTime);
    void update();
    bool externalEvent(int type) override;
    // 通过FreeRTOS任务执行jump，避免阻塞
    static void jumpSoundTask(void *args);    
    static void alarmTickCallback();
};
