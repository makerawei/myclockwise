#include "CWDateTime.h"

void CWDateTime::begin(const char *timeZone, bool use24format, const char *ntpServer = NTP_SERVER, const char *posixTZ = "", const char *alarmClockStr = NULL)
{
  Serial.printf("[Time] NTP Server: %s, Timezone: %s\n", ntpServer, timeZone);
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
  this->setAlarm(alarmClockStr);
  waitForSync(10);
}

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

bool CWDateTime::is24hFormat() 
{
  return this->use24hFormat;
}

bool CWDateTime::setAlarm(const char     *alarmClockStr) {
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
