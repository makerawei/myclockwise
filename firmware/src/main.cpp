#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
// Clockface
#include <Clockface.h>
// Commons
#include <WiFiController.h>
#include <CWDateTime.h>
#include <CWPreferences.h>
#include <CWWebServer.h>
#include <StatusController.h>
#include <AudioHelper.h>

#define ESP32_LED_BUILTIN 2
#define USER_BUTTON_PIN 36
#define BUTTON_DEBOUNCE_DELAY 200

MatrixPanel_I2S_DMA *dma_display = nullptr;

Clockface *clockface;

WiFiController wifi;
//CWDateTime cwDateTime;

volatile bool buttonPressed = false;
volatile unsigned long lastDebounceTime = 0;


void displaySetup(bool swapBlueGreen, uint8_t displayBright, uint8_t displayRotation)
{
  HUB75_I2S_CFG mxconfig(64, 64, 1);

  if (swapBlueGreen)
  {
    // Swap Blue and Green pins because the panel is RBG instead of RGB.
    mxconfig.gpio.b1 = 26;
    mxconfig.gpio.b2 = 12;
    mxconfig.gpio.g1 = 27;
    mxconfig.gpio.g2 = 13;
  }

  mxconfig.gpio.e = 18;
  mxconfig.clkphase = false;

  // Display Setup
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(displayBright);
  dma_display->clearScreen();
  dma_display->setRotation(displayRotation);
}

void setup()
{
  Serial.begin(115200);
  pinMode(ESP32_LED_BUILTIN, OUTPUT);
  pinMode(USER_BUTTON_PIN, INPUT);
  
  AudioHelper::getInstance()->begin();
  StatusController::getInstance()->blink_led(5, 100);

  ClockwiseParams::getInstance()->load();

  pinMode(ClockwiseParams::getInstance()->ldrPin, INPUT);

  displaySetup(ClockwiseParams::getInstance()->swapBlueGreen, ClockwiseParams::getInstance()->displayBright, ClockwiseParams::getInstance()->displayRotation);
  clockface = new Clockface(dma_display);

  StatusController::getInstance()->clockwiseLogo();
  delay(1000);

  StatusController::getInstance()->wifiConnecting();
  if (wifi.begin())
  {
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

void onSerialControl() {
  int n = Serial.available();
  if(n > 0) {
    char cmd[32] = {0};
    size_t size = Serial.readBytes(cmd, n);
    cmd[size - 1] = '\0';
    if(strcmp(cmd, "jump") == 0) {
      clockface->externalEvent(0);
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

void loop()
{
  wifi.handleImprovWiFi();

  /*
  if (wifi.isConnected())
  {
    ClockwiseWebServer::getInstance()->handleHttpRequest();
  }
  */

  if (wifi.connectionSucessfulOnce)
  {
    clockface->loop();
  }

  onSerialControl();
  onButtonEvent();
}
