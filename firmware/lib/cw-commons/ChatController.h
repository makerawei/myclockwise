#pragma once
#include <Arduino.h>
#include <SPIFFS.h>
#include <HTTPClient.h>

#define DEFAULT_TIMEOUT 4000 // ms
#ifndef WAV_FILE
#define WAV_FILE "/record2.wav"
#endif

struct ChatController {
  HTTPClient http;
  static ChatController *getInstance() {
    static ChatController base;
    return &base;
  }

  void chatToServer() {
    File file = SPIFFS.open(WAV_FILE, FILE_READ);
    if(!file){
      Serial.println("File not found");
      return;
    }
    http.begin("http://47.115.38.84:8000/upload-wav");
    http.setTimeout(DEFAULT_TIMEOUT);
    http.addHeader("Content-Type", "multipart/form-data");
    http.addHeader("Device-Id", "123456789");
    Serial.println("start to upload");
    int statusCode = http.sendRequest("POST", &file, file.size());
    if(statusCode != 200) {
      Serial.printf("request error:%d\n", statusCode);
    } else {
      String response = http.getString();
      Serial.printf("response:%s\n", response.c_str());
    }
  }
};


