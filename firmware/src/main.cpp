#include <Arduino.h>
#include <Clockface.h>
#include <WiFiController.h>
#include <CWDateTime.h>
#include <CWPreferences.h>
#include <CWWebServer.h>
#include <StatusController.h>
#include <AudioHelper.h>
#include <DisplayController.h>
#include <ButtonController.h>

#define ESP32_LED_BUILTIN 2

Clockface *clockface;
WiFiController wifi;
/*
volatile bool buttonPressed = false;
volatile unsigned long lastDebounceTime = 0;
*/

void onButtonClicked() {
  clockface->externalEvent(0);
}

void onButtonLongPressStart() {
  Serial.println("button long press start");
}

void onButtonLongPressStop() {
  Serial.println("button long press stop");
}

void setup() {
  Serial.begin(115200);
  pinMode(ESP32_LED_BUILTIN, OUTPUT);
  
  ButtonController::getInstance()->begin(onButtonClicked, onButtonLongPressStart, onButtonLongPressStop);
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


/*
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
*/

void loop() {
  /*
  if (wifi.isConnected())
  {
    ClockwiseWebServer::getInstance()->handleHttpRequest();
  }
  */
  ButtonController::getInstance()->loop();
  wifi.handleImprovWiFi();
  if(wifi.connectionSucessfulOnce) {
    clockface->loop();
  }
  //onButtonEvent();
}
