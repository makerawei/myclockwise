#pragma once
#include <Arduino.h>

#include <ezTime.h>
#include <WiFi.h>

#define MAX_ALARM_CLOCK_COUNT 5

typedef struct {
  int hour;
  int minute;
  int alarmDuration; // 闹铃持续时长
  uint8_t style;     // 闹铃方式
  bool triggered;    // 是否已触发通知（默认边沿触发方式，到点只通知一次）
} AlarmClock;

class CWDateTime
{
private:
  Timezone myTZ;
  bool use24hFormat = true;
  int alarmClockCount = 0;
  AlarmClock alarmClocks[MAX_ALARM_CLOCK_COUNT];
  
public:
  static CWDateTime *getInstance() {
    static CWDateTime base;
    return &base;
  }

  void begin(const char *timeZone, bool use24format, const char *ntpServer, const char *posixTZ, const char *alarmClockStr);
  String getFormattedTime();
  String getFormattedTime(const char* format);

  char *getHour(const char *format);
  char *getMinute(const char *format);
  int getHour();
  int getMinute();
  int getSecond();
  long getMilliseconds();

  int getDay();
  int getMonth();
  int getWeekday();

  bool isAM();
  bool is24hFormat();

  void triggerAlarm(const int index);
  bool setAlarm(const char     *alarmClockStr); // 格式：HH:MM 多个时间以空格间隔，最多设置5个，示例：08:00 12:00 14:30
  bool checkAlarm();
};
