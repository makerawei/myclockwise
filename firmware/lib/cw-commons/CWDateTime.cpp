#include "CWDateTime.h"
#include <WiFiUdp.h>
#include "TzOffsetHelper.h"

static const char *TAG = "CWDateTime";

static WiFiUDP ntpUDP;


void CWDateTime::begin(const char *timeZone, bool use24format, const char *ntpServer = NTP_SERVER, const char *posixTZ = "", const char *alarmClockStr = NULL)
{
  int offset = TzOffsetHelper::offset(timeZone);
  Serial.printf("[Time] NTP Server: %s, Timezone: %s, UTC_offset: %d\n", ntpServer, timeZone, offset);
  rtc = new ESP32Time(offset);
  ntp = new NTPClient(ntpUDP, ntpServer, 0, 600); // 每间隔10分钟同步一次NTP服务器
  ntp->begin();
  if(ntp->forceUpdate()) {
    rtc->setTime(ntp->getEpochTime());
    Serial.printf("[%s]NTP update time success, current time is %s\n", TAG, getFormattedTime().c_str());
  } else {
    Serial.printf("[%s]NTP update time failed\n", TAG);
  }
#if 0
  ezt::setServer(String(ntpServer));
  if (strlen(posixTZ) > 1) {
    // An empty value still contains a null character so not empty is a value greater than 1.
    // Set to defined Posix TZ
    myTZ.setPosix(posixTZ);
  } else {
    // Use automatic eztime remote lookup
    myTZ.setLocation(timeZone);
  }

  this->use24hFormat = use24format;
  ezt::updateNTP();
  waitForSync(10);
#endif
  
  this->setAlarm(alarmClockStr);

}

void CWDateTime::updateNTP() {
  if(WiFi.status() == WL_CONNECTED) {
    if(ntp->update()) {
      rtc->setTime(ntp->getEpochTime());
    }
  }
}

String CWDateTime::getFormattedTime()
{
  return rtc->getTime();
  //return myTZ.dateTime();
}

String CWDateTime::getFormattedTime(const char *format)
{
  return rtc->getTime(format);
  //return myTZ.dateTime(format);
}

char *CWDateTime::getHour(const char *format)
{
  static char buffer[3] = {'\0'};
  snprintf(buffer, sizeof(buffer), "%02d", rtc->getHour(use24hFormat));
  //strncpy(buffer, myTZ.dateTime((use24hFormat ? "H" : "h")).c_str(), sizeof(buffer));
  return buffer;
}

char *CWDateTime::getMinute(const char *format)
{
  static char buffer[3] = {'\0'};
  snprintf(buffer, sizeof(buffer), format, rtc->getMinute());
  //strncpy(buffer, myTZ.dateTime("i").c_str(), sizeof(buffer));
  return buffer;
}

int CWDateTime::getHour()
{
  //return myTZ.dateTime((use24hFormat ? "H" : "h")).toInt();
  return rtc->getHour(use24hFormat);
}

int CWDateTime::getMinute()
{
  //return myTZ.dateTime("i").toInt();
  return rtc->getMinute();
}

int CWDateTime::getSecond()
{
  //return myTZ.dateTime("s").toInt();
  return rtc->getSecond();
}

int CWDateTime::getDay() 
{
  //return myTZ.dateTime("d").toInt();
  return rtc->getDay();
}
int CWDateTime::getMonth()
{
  //return myTZ.dateTime("m").toInt();
  return rtc->getMonth();
}
int CWDateTime::getWeekday() 
{
  //return myTZ.dateTime("w").toInt()-1;
  return rtc->getDayofWeek() + 1; // 1 - 7
}

long CWDateTime::getMilliseconds() 
{
  //return myTZ.ms(TIME_NOW);
  return rtc->getMillis();
}

bool CWDateTime::isAM() 
{
  //return myTZ.isAM();
  return rtc->getAmPm() == "AM";
}


#if 0
String CWDateTime::getFormattedTime()
{
  return myTZ.dateTime();
}

String CWDateTime::getFormattedTime(const char *format)
{
  return myTZ.dateTime(format);
}

char *CWDateTime::getHour(const char *format)
{
  static char buffer[3] = {'\0'};
  strncpy(buffer, myTZ.dateTime((use24hFormat ? "H" : "h")).c_str(), sizeof(buffer));
  return buffer;
}

char *CWDateTime::getMinute(const char *format)
{
  static char buffer[3] = {'\0'};
  strncpy(buffer, myTZ.dateTime("i").c_str(), sizeof(buffer));
  return buffer;
}

int CWDateTime::getHour()
{
  return myTZ.dateTime((use24hFormat ? "H" : "h")).toInt();
}

int CWDateTime::getMinute()
{
  return myTZ.dateTime("i").toInt();
}

int CWDateTime::getSecond()
{
  return myTZ.dateTime("s").toInt();
}

int CWDateTime::getDay() 
{
  return myTZ.dateTime("d").toInt();
}
int CWDateTime::getMonth()
{
  return myTZ.dateTime("m").toInt();
}
int CWDateTime::getWeekday() 
{
  return myTZ.dateTime("w").toInt()-1;
}

long CWDateTime::getMilliseconds() 
{
  return myTZ.ms(TIME_NOW);
}

bool CWDateTime::isAM() 
{
  return myTZ.isAM();
}

#endif

bool CWDateTime::is24hFormat() 
{
  return this->use24hFormat;
}

bool CWDateTime::setAlarm(const char *alarmClockStr) {
  if(alarmClockStr == NULL) {
    return false;
  }
  const char *delim = ";"; // 分隔符
  char *str = strdup(alarmClockStr);
  bool isFirst = true;
  this->alarmClockCount = 0;
  while(this->alarmClockCount < MAX_ALARM_CLOCK_COUNT) {
    char *token = strtok(isFirst ? str : NULL, delim);
    if(token == NULL) {
      Serial.println("token is NULL");
      break;
    }
    Serial.printf("token is %s\n", token);
    isFirst = false;
    String alarmClockStr = String(token);
    int colonPos = alarmClockStr.indexOf(':');
    if(colonPos < 1 || colonPos >= alarmClockStr.length() - 1) {
      continue;
    }
    int hour = alarmClockStr.substring(0, colonPos).toInt();
    int munite = alarmClockStr.substring(colonPos + 1).toInt();
    if(hour >= 0 && hour <= 23 && munite >= 0 && munite<= 59) {
      Serial.printf("set alarm clock at %02d:%02d\n", hour, munite);
      AlarmClock *alarmClock = &this->alarmClocks[this->alarmClockCount++];
      alarmClock->hour = hour;
      alarmClock->minute = munite;
      alarmClock->alarmDuration = 60;
      alarmClock->style = 0;
      alarmClock->triggered = false;
    } else {
      continue;
    }
  }
  Serial.printf("alarm clock count is %d\n", this->alarmClockCount);
  free(str);
  return true;
}

int CWDateTime::checkAlarm() {
  for(int i = 0; i < this->alarmClockCount; i++) {
    int hour = getHour();
    int mutite = getMinute();
    if(hour == alarmClocks[i].hour && mutite == alarmClocks[i].minute && !alarmClocks[i].triggered) {
      alarmClocks[i].triggered = true;
      return i;
    }
  }

  return -1;
}

void CWDateTime::resetAlarm(const int index) {
  if(index >= 0 && index < this->alarmClockCount) {
    alarmClocks[index].triggered = false;
  }
}
