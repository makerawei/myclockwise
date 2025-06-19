#pragma once
#include <Arduino.h>
#include <CWPreferences.h>
#include <SPIFFS.h>
#include <driver/i2s.h>
#include "ChatController.h"

#define WAV_FILE "/record3.wav"
#define WAV_HEADER_SIZE   44

volatile bool recording = false;

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
  
  static void buildWavHeader(uint8_t* header, int wavSize){
    header[0] = 'R';
    header[1] = 'I';
    header[2] = 'F';
    header[3] = 'F';
    unsigned int fileSize = wavSize + WAV_HEADER_SIZE - 8;
    header[4] = (uint8_t)(fileSize & 0xFF);
    header[5] = (uint8_t)((fileSize >> 8) & 0xFF);
    header[6] = (uint8_t)((fileSize >> 16) & 0xFF);
    header[7] = (uint8_t)((fileSize >> 24) & 0xFF);
    header[8] = 'W';
    header[9] = 'A';
    header[10] = 'V';
    header[11] = 'E';
    header[12] = 'f';
    header[13] = 'm';
    header[14] = 't';
    header[15] = ' ';
    header[16] = 0x10; // linear PCM
    header[17] = 0x00;
    header[18] = 0x00;
    header[19] = 0x00;
    header[20] = 0x01; // linear PCM
    header[21] = 0x00;
    header[22] = 0x01; // monoral
    header[23] = 0x00;
    header[24] = 0x80; // sampling rate 16000
    header[25] = 0x3E;
    header[26] = 0x00;
    header[27] = 0x00;
    header[28] = 0x00;
    header[29] = 0x7D;
    header[30] = 0x00; //0x01
    header[31] = 0x00;
    header[32] = 0x02; // 16bit monoral
    header[33] = 0x00;
    header[34] = 0x10; // 16bit
    header[35] = 0x00;
    header[36] = 'd';
    header[37] = 'a';
    header[38] = 't';
    header[39] = 'a';
    header[40] = (uint8_t)(wavSize & 0xFF);
    header[41] = (uint8_t)((wavSize >> 8) & 0xFF);
    header[42] = (uint8_t)((wavSize >> 16) & 0xFF);
    header[43] = (uint8_t)((wavSize >> 24) & 0xFF);
  }


  static void dataScale(uint8_t * d_buff, uint8_t* s_buff, uint32_t len) {
    uint32_t j = 0;
    uint32_t dac_value = 0;
    for (int i = 0; i < len; i += 2) {
      dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
      d_buff[j++] = 0;
      d_buff[j++] = dac_value * 256 / 4096;
    }
  }

  static void recordTask(void *param) {
    size_t audioSize = 0;
    int16_t pcmBuffer[DMA_BUF_LEN] = {0};
    int16_t wavBuffer[DMA_BUF_LEN] = {0};
    
    SPIFFS.remove(WAV_FILE);
    File fp = SPIFFS.open(WAV_FILE, FILE_WRITE);
    if(!fp) {
      Serial.println("fail to create file");
      goto end;
    }

    Serial.println("===> recording...");
    fp.seek(WAV_HEADER_SIZE);    
    I2SController::getInstance()->switchToRx();
    I2SController::getInstance()->switchLock(); // 等待switchToRx切换完成
    while(recording) {
      size_t size = 0;
      esp_err_t ret = i2s_read(I2S_PORT, pcmBuffer, DMA_BUF_LEN / sizeof(pcmBuffer[0]), &size, portMAX_DELAY); // pdMS_TO_TICKS(10)
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
        dataScale((uint8_t*)wavBuffer, (uint8_t*)pcmBuffer, size);
        fp.write((uint8_t *)wavBuffer, size);
        audioSize += size;
      }
    }
    I2SController::getInstance()->switchUnlock();
    uint8_t header[WAV_HEADER_SIZE];
    buildWavHeader(header, audioSize);
    fp.seek(0);
    fp.write(header, WAV_HEADER_SIZE);
    fp.close();
    Serial.printf("record finished, audio size is %d\n", audioSize);
    ChatController::getInstance()->chatToServer();
end:
    recording = false;
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


