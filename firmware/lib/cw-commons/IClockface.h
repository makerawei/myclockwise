#pragma once

#include <Adafruit_GFX.h>
#include "AudioHelper.h"
#include "CWDateTime.h"

typedef void (*AlarmTickCallbackType)(void);

class IClockface {
protected:
    Adafruit_GFX* _display;
    CWDateTime* _dateTime;
    int _alarmIndex; // 当前触发闹钟的索引
    TaskHandle_t _xAlarmTaskHandle;    
    TimerHandle_t _alarmTimer;
    TickType_t _xLastAlarmTime;
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
    
    void updateTime();
    bool alarmStarts();
    bool isAlarmTaskRunning();
    void tryToCancelAlarmTask();

    static void alarmSetSoundUrl(String url);
    static void alarmSetTickFunc(AlarmTickCallbackType func);
    static void alarmTask(void *args);
    static void alarmTimerCallback(TimerHandle_t xTimer);
    
    virtual void setup(CWDateTime *dateTime) = 0;
    virtual void update() = 0;

};
