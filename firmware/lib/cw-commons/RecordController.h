#pragma once
#include <Arduino.h>
#include <driver/i2s.h>
#include <WebSocketsClient.h>

extern WebSocketsClient webSocket;
volatile bool recording = false;
const uint8_t RECORD_BEGIN_BUFF[] = {'\xBB', '\xBB'};
const uint8_t RECORD_END_BUFF[] = {'\xEE', '\xEE'};


struct RecordController {
  static RecordController *getInstance() {
    static RecordController base;
    return &base;
  }

  void begin() {

  }

  bool isRecording() {
    return recording;
  }

  
  static void recordTask(void *param) {
    size_t audioSize = 0;
    int32_t audioBuffer[DMA_BUF_LEN] = {0};
    int16_t pcmBuffer[DMA_BUF_LEN] = {0};

    Serial.println("===> recording...");
    I2SController::getInstance()->switchToRx();
    I2SController::getInstance()->switchLock(); // 等待switchToRx切换完成

    webSocket.sendBIN(RECORD_BEGIN_BUFF, sizeof(RECORD_BEGIN_BUFF));
    
    while(recording) {
      size_t size = 0;
      esp_err_t ret = i2s_read(I2S_PORT, audioBuffer, DMA_BUF_LEN / sizeof(audioBuffer[0]), &size, portMAX_DELAY); // pdMS_TO_TICKS(10)
      if (ret != ESP_OK ) {
        if(ret == ESP_ERR_INVALID_STATE) {
          i2s_stop(I2S_PORT);
          i2s_start(I2S_PORT); // 尝试恢复
        } else {
          Serial.printf("i2s_read error:%d\n", ret);
          break;
        }
      } else if(size <= 0) {
        vTaskDelay(1);
      } else {
        int samples = size / sizeof(audioBuffer[0]);
        for(int i = 0; i < samples; i++) {
          pcmBuffer[i] = (int16_t)(audioBuffer[i] >> 16);
        }
        webSocket.sendBIN((uint8_t*)pcmBuffer, samples * sizeof(pcmBuffer[0]));
        audioSize += size;
      }
    }

    recording = false;
    
    I2SController::getInstance()->switchUnlock();
    Serial.printf("record finished, audio size is %d\n", audioSize);
    webSocket.sendBIN(RECORD_END_BUFF, sizeof(RECORD_END_BUFF));
    
    // 录音完成后需要主动切换到音频播放模式
    I2SController::getInstance()->switchToTx();
    
    vTaskDelete(NULL);
  }

  void stopRecord() {
    Serial.println("user stop record");
    recording = false;
  }
  
  bool startRecord() {
    TaskHandle_t handle = NULL;    
    xTaskCreatePinnedToCore(
      recordTask,
      "recordTask", 
      8 * 1024,
      NULL,
      2, 
      &handle,
      1
    );

    recording = handle != NULL;
    return recording;
  }
};


