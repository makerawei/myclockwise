#pragma once

#include <Adafruit_GFX.h>
#include "AudioHelper.h"
#include "CWDateTime.h"

#define MIN_BRIGHT_DISPLAY_ON 4
#define MIN_BRIGHT_DISPLAY_OFF 0
#define NIGHT_MODE_BRIGHTNESS 10

typedef void (*AlarmTickCallbackType)(void);

class IClockface {
protected:
    Adafruit_GFX* _display;
    CWDateTime* _dateTime;
    int _alarmIndex; // 当前触发闹钟的索引
    TaskHandle_t _xAlarmTaskHandle;    
    TimerHandle_t _alarmTimer;
    TickType_t _xLastAlarmTime;
    bool _nightMode; // 是否启动夜间模式
    static AlarmTickCallbackType _tickFunc;
    static SemaphoreHandle_t _semaphore;
    static String _alarmSoundUrl;
    
public:
    IClockface(Adafruit_GFX* display);
    virtual ~IClockface() {}
    virtual bool externalEvent(int type) {
      tryToCancelAlarmTask();
      return true;
    }

    virtual void automaticBrightControl();
    // 夜间模式的初始化和更新函数保持默认
    void setupNightMode();
    void updateNightMode();

    // 正常模式下的初始化和更新函数需要不同的表盘自己实现代码
    virtual void setup() = 0;
    virtual void update() = 0;

    void init();
    void loop();
    
    void updateTime();
    bool alarmStarts();
    bool isAlarmTaskRunning();
    void tryToCancelAlarmTask();

    static void alarmSetSoundUrl(String url);
    static void alarmSetTickFunc(AlarmTickCallbackType func);
    static void alarmTask(void *args);
    static void alarmTimerCallback(TimerHandle_t xTimer);
};
