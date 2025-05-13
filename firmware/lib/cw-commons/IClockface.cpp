#include "IClockface.h"
#include <Locator.h>
#include <EventBus.h>

EventBus eventBus;
SemaphoreHandle_t IClockface::_semaphore = NULL;
AlarmTickCallbackType IClockface::_tickFunc = NULL;


IClockface::IClockface(Adafruit_GFX* display) {
  _display = display;
  _alarmTimer = NULL;
  _alarmIndex = -1;
  Locator::provide(display);
  Locator::provide(&eventBus);
}

void IClockface::updateTime() {
  _dateTime->updateNTP();
}


bool IClockface::isAlarmTaskRunning() {
  return _alarmIndex >= 0;
}

void IClockface::alarmSetTickFunc(AlarmTickCallbackType func) {
  Serial.println("set alram tick func success");
  IClockface::_tickFunc = func;
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
  String url = AUDIO_MARIO_START;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  while(true) {
    vTaskDelay(pdMS_TO_TICKS(100));
    AudioHelper::getInstance()->play(url);
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




