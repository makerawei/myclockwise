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
#include <RecordController.h>
#include <WebSocketsClient.h>
#include <CommandController.h>

#define ESP32_LED_BUILTIN 2
#define WEBSOCKET_SERVER  "47.115.38.84"
#define WEBSOCKET_PORT    8000
#define WEBSOCKET_AUDIO_PATH "/audio"


Clockface *clockface;
WiFiController wifi;
WebSocketsClient webSocket;

/*
volatile bool buttonPressed = false;
volatile unsigned long lastDebounceTime = 0;
*/

void onButtonClicked() {
  clockface->externalEvent(0);
}

void onButtonLongPressStart() {
  Serial.println("button long press start");
  RecordController::getInstance()->startRecord();
}

void onButtonLongPressStop() {
  Serial.println("button long press stop");
  RecordController::getInstance()->stopRecord();
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_PING:
      break;
    case WStype_PONG:
      break;
    case WStype_DISCONNECTED:
      Serial.println("[WSc] Disconnected!");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);
      if(CommandController::getInstance()->handleCommand((const char *)payload)) {
        Serial.println("[WSc] command handled successfully");
        AudioHelper::getInstance()->success();
      } else {
        Serial.println("[WSc] command handling failed");
      }
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ESP32_LED_BUILTIN, OUTPUT);
  
  ButtonController::getInstance()->begin(onButtonClicked, onButtonLongPressStart, onButtonLongPressStop);
  RecordController::getInstance()->begin();
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
      webSocket.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, WEBSOCKET_AUDIO_PATH);
      webSocket.onEvent(webSocketEvent);
      webSocket.setReconnectInterval(3000);
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
  webSocket.loop();
  ButtonController::getInstance()->loop();
  wifi.handleImprovWiFi();
  if(wifi.connectionSucessfulOnce) {
    clockface->loop();
  }
  //onButtonEvent();
}
