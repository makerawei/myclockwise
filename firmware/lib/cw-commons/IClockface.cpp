#include "IClockface.h"
#include <Locator.h>
#include <EventBus.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "FreeSerifBold9pt7b.h"

EventBus eventBus;
SemaphoreHandle_t IClockface::_semaphore = NULL;
AlarmTickCallbackType IClockface::_tickFunc = NULL;
String IClockface::_alarmSoundUrl = "";

static unsigned long lastMillis = 0;


IClockface::IClockface(Adafruit_GFX* display) {
  _display = display;
  _dateTime = CWDateTime::getInstance();
  _alarmTimer = NULL;
  _alarmIndex = -1;
  _nightMode = false;
  Locator::provide(display);
  Locator::provide(&eventBus);
  _semaphore = xSemaphoreCreateBinary();
}

void IClockface::init() {
  if(_nightMode) {
    setupNightMode();
  } else {
    setup();
  }
}
void IClockface::loop() {
  if(_nightMode) {
    updateNightMode();
  } else {
    update();
  }
}

void IClockface::updateTime() {
  _dateTime->updateNTP();
  const int _alarmIndex = _dateTime->checkAlarm();
  if(_alarmIndex >= 0) {
    this->_alarmIndex = _alarmIndex;
    alarmStarts();
  }
}

void IClockface::setupNightMode() {
  Locator::getDisplay()->setFont(&FreeSerifBold9pt7b);
  Locator::getDisplay()->fillRect(0, 0, 64, 64, 0);
  Locator::getDisplay()->setTextColor(0xf800);
  ((MatrixPanel_I2S_DMA *)Locator::getDisplay())->setBrightness8(10);
}

void IClockface::updateNightMode() {
  if(millis() - lastMillis < 1000) {
    return;
  }
  static char preTimeStr[10] = {0};
  lastMillis = millis();
  char timeStr[10] = {0};
  snprintf(timeStr, sizeof(timeStr), "%s:%s", _dateTime->getHour("%02d"), _dateTime->getMinute("%02d"));
  if(strcmp(preTimeStr, timeStr) == 0) {
    return;
  }
  int16_t x, y;
  uint16_t w, h;
  strncpy(preTimeStr, timeStr, sizeof(preTimeStr) - 1);
  Locator::getDisplay()->fillRect(0, 0, 64, 64, 0);
  Locator::getDisplay()->getTextBounds(timeStr, 0, 0, &x, &y, &w, &h);
  Locator::getDisplay()->setCursor(32 - (w / 2), 32);
  Locator::getDisplay()->print(timeStr);
  this->updateTime();
}

bool IClockface::isAlarmTaskRunning() {
  return _alarmIndex >= 0;
}

void IClockface::alarmSetTickFunc(AlarmTickCallbackType func) {
  Serial.println("set alram tick func success");
  IClockface::_tickFunc = func;
}

void IClockface::alarmSetSoundUrl(String url) {
  IClockface::_alarmSoundUrl = url;
}

void IClockface::alarmTimerCallback(TimerHandle_t xTimer) {
  IClockface *self = (IClockface *)pvTimerGetTimerID(xTimer);
  TickType_t xCurrentTime = xTaskGetTickCount();
  TickType_t elapsedTicks = xCurrentTime - self->_xLastAlarmTime;
  uint32_t elapsedMs = elapsedTicks * portTICK_PERIOD_MS;
  if(elapsedMs < MAX_ALARM_DURATION_MS) {
    if(IClockface::_tickFunc) {
      IClockface::_tickFunc();
    }
  } else {
    if(self->_alarmTimer) {
      if(xTimerDelete(self->_alarmTimer, pdMS_TO_TICKS(100)) == pdPASS) {
        self->_alarmTimer = NULL;
      }
    }
    Serial.printf("stop alarm[%d] in timer callback\n", self->_alarmIndex);
    self->tryToCancelAlarmTask();
  }
}

void IClockface::tryToCancelAlarmTask() {
  Serial.println("try to cancel alarm task");
  if(isAlarmTaskRunning()) {
    Serial.println("alarm task is running, cancel it");
    if(_alarmTimer != NULL) {
      if(xTimerDelete(_alarmTimer, pdMS_TO_TICKS(100)) == pdPASS) {
        _alarmTimer = NULL;
      }
    }
    _alarmIndex = -1;
    Serial.println("===> cancel alarm task");
    _dateTime->resetAlarm(_alarmIndex);
    AudioHelper::getInstance()->setStopFlag(true);
    xTaskNotify(_xAlarmTaskHandle, 1, eSetValueWithOverwrite);
    if(xSemaphoreTake(_semaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
      Serial.println("alarm xSemaphoreTake done");
    }
  } else {
    Serial.println("alarm task is not running, just ignore");
  }
}

void IClockface::alarmTask(void *pvParams) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  while(true) {
    vTaskDelay(1);
    AudioHelper::getInstance()->play(IClockface::_alarmSoundUrl);
    TickType_t xCurrentTime = xTaskGetTickCount();
    TickType_t elapsedTicks = xCurrentTime - xLastWakeTime;
    uint32_t elapsedMs = elapsedTicks * portTICK_PERIOD_MS;
    if(elapsedMs >= MAX_ALARM_DURATION_MS) {
      break;
    } else {
      uint32_t notificationValue;
      if(xTaskNotifyWait(0, ULONG_MAX, &notificationValue, pdMS_TO_TICKS(10)) == pdTRUE) {
        if(notificationValue == 1) {
          Serial.println("recved cancel alarm task notify");
          break;
        }
      }
    }
  }

  xSemaphoreGive(_semaphore);
  IClockface *self = (IClockface *)pvParams;
  if(self) {
    Serial.println("set _xAlarmTaskHandle as NULL");
    self->_alarmIndex = -1;
  }

  vTaskDelete(NULL);
}


bool IClockface::alarmStarts() {
  if(_alarmTimer) {
    Serial.println("alarm already started");
    return true;
  }
  _alarmTimer = xTimerCreate(
    "alarmTimer",
    pdMS_TO_TICKS(1000),
    pdTRUE,
    (void *)this,
    alarmTimerCallback
  );
  if(_alarmTimer == NULL) {
    Serial.println("alarm starts error: unable to create alarm timer!");
    return false;
  }
  _xLastAlarmTime = xTaskGetTickCount();
  xTimerStart(_alarmTimer, pdMS_TO_TICKS(100));
  
  xTaskCreatePinnedToCore(
    &IClockface::alarmTask,
    "alarmTask", 
    10240,
    (void *)this,
    1, 
    &_xAlarmTaskHandle,
    1 
  );
  return _xAlarmTaskHandle != NULL;
}




