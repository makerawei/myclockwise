#include <Arduino.h>
#include <Clockface.h>
#include <WiFiController.h>
#include <CWDateTime.h>
#include <CWPreferences.h>
#include <CWWebServer.h>
#include <StatusController.h>
#include <AudioHelper.h>
#include <DisplayController.h>

#define ESP32_LED_BUILTIN 2
#define USER_BUTTON_PIN 36
#define BUTTON_DEBOUNCE_DELAY 200

Clockface *clockface;
WiFiController wifi;
volatile bool buttonPressed = false;
volatile unsigned long lastDebounceTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(ESP32_LED_BUILTIN, OUTPUT);
  pinMode(USER_BUTTON_PIN, INPUT);
  
  AudioHelper::getInstance()->begin();
  StatusController::getInstance()->blink_led(5, 100);

  ClockwiseParams::getInstance()->load();

  pinMode(ClockwiseParams::getInstance()->ldrPin, INPUT);

  DisplayController::getInstance()->begin();
  clockface = new Clockface(DisplayController::getInstance()->getDmaDisplay());

  StatusController::getInstance()->clockwiseLogo();
  delay(1000);

  StatusController::getInstance()->wifiConnecting();
  if (wifi.begin()) {
    StatusController::getInstance()->ntpConnecting();
    CWDateTime::getInstance()->begin(ClockwiseParams::getInstance()->timeZone.c_str(), 
        ClockwiseParams::getInstance()->use24hFormat, 
        ClockwiseParams::getInstance()->ntpServer.c_str(),
        ClockwiseParams::getInstance()->manualPosix.c_str(),
        ClockwiseParams::getInstance()->alarmClock.c_str());
    clockface->init();
    if(wifi.isConnected()) {
      ClockwiseWebServer::getInstance()->handleHttpRequestInTask();
    }
  }
}

void onButtonEvent() {
  bool value = digitalRead(USER_BUTTON_PIN);
  if(!value) {
    if(!buttonPressed) {
      unsigned long currentTime = millis();
      if ((currentTime - lastDebounceTime) > BUTTON_DEBOUNCE_DELAY) {
        buttonPressed = true;
        lastDebounceTime = currentTime;
        clockface->externalEvent(0);
      }
    }
  } else {
    if(buttonPressed) {
      buttonPressed = false;
    }
  }
}

void loop() {
  /*
  if (wifi.isConnected())
  {
    ClockwiseWebServer::getInstance()->handleHttpRequest();
  }
  */
  wifi.handleImprovWiFi();
  if(wifi.connectionSucessfulOnce) {
    clockface->loop();
  }
  onButtonEvent();
}
