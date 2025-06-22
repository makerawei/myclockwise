#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <driver/i2s.h>
#include <I2SController.h>

#define WAV_HEAD_SIZE 44
#define SUCCESS_SOUND_URL "https://makerawei-1251006064.cos.ap-guangzhou.myqcloud.com/clockwise/human_success.wav"

#define WRITE_TO_FILE(file, buffer, size) do { \
  if(file) { \
    file.write(buffer, size); \
  } \
} while(0)
static bool isInited = false;
static bool spiffsInited = false;
static volatile bool stopFlag = false;

struct AudioHelper {
  static AudioHelper *getInstance() {
    static AudioHelper base;
    return &base;
  }
  
  bool begin() {
    if (isInited) {
      return true;
    }
    spiffsInited = SPIFFS.begin(true);
    Serial.println(spiffsInited ? "SPIFFS init success" : "SPIFFS init failed");
    return isInited;
  }

  static uint32_t url_hash(const char *str) {
    uint32_t hash = 5381;
    int c;
  
    while ((c = *str++)) {
      hash = ((hash << 5) + hash) + c;
    }
    if (strlen(str) == 8) {
      return hash & 0xFF;
    } else  {
      return hash & 0xFFFF;
    }
  }

  void stop() {
    i2s_zero_dma_buffer(I2S_PORT);  // 清空DMA缓存
    const int zeroSamples = 1024;
    int32_t zeroData = 0;
    for (int i = 0; i < zeroSamples; i++) {
      size_t bytesWritten = 0;
      i2s_write(I2S_PORT, &zeroData, sizeof(zeroData), &bytesWritten,
                portMAX_DELAY);
    }
    Serial.println("audio stopped");
  }

  void setStopFlag(bool flag) {
    stopFlag = flag;
  }

  // 立即静音
  // 该函数可能i2s_stop或i2s_start时卡住，暂时不要使用
  void emergencyMute() {
    Serial.println("===> emergency mute");
    i2s_stop(I2S_PORT);
    // I2S0.conf.tx_fifo_reset = 1;
    // I2S0.conf.tx_fifo_reset = 0;
    i2s_zero_dma_buffer(I2S_PORT);
    i2s_start(I2S_PORT);
    vTaskDelay(pdMS_TO_TICKS(20));
  }

  void write(const int16_t *buffer, const size_t size) {
    for (size_t i = 0; i < size / sizeof(int16_t); i++) {
      size_t bytesWritten = 0;
      i2s_write(I2S_PORT, &buffer[i], sizeof(buffer[i]), &bytesWritten,
                portMAX_DELAY);
    }
  }

  static void playerTask(void *pvParams) {
    String url = String((char *)pvParams);
    Serial.printf("play url:%s\n", url.c_str());
    if(pvParams) {
      while(true) {
        AudioHelper::getInstance()->play(url);
        break;
      }
    }
    vTaskDelete(NULL);
  }

  static void play(TaskFunction_t playFunc, const char *url, const int core=0) {
    xTaskCreatePinnedToCore(
      playFunc ? playFunc : &AudioHelper::playerTask,
      "playTask", 
      8192,
      url != NULL ? (void *)url: NULL,
      1, 
      NULL,
      core
    );
  }

  void play(const int16_t *buffer, const size_t size) {}

  void play(String url) {
    char filePath[32] = {0};
    if(!I2SController::getInstance()->isTxMode()) { // 如果正在录音则不播放音频
      return;
    }
    snprintf(filePath, sizeof(filePath), "/%d.wav", url_hash(url.c_str()));
    if(!play(filePath)) {
      download(url);
    }
  }
  
  void success() {
    play(String(SUCCESS_SOUND_URL));
  }

  void download(String url, bool play=false) {
    if (WiFi.status() != WL_CONNECTED) {
      return;
    }
    Serial.println("wifi connected, start to download audio...");
    HTTPClient http;
    http.addHeader("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/133.0.0.0 Safari/537.36");
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
      Serial.printf("invalid httpCode:%d\n", httpCode);
      return;
    }
    File file;
    char filePath[32] = {0};
    snprintf(filePath, sizeof(filePath), "/%d.wav", url_hash(url.c_str()));
    Serial.printf("cache filePath is %s\n", filePath);
    file = SPIFFS.open(filePath, FILE_WRITE);
    if(!file) {
      Serial.println("open cache file failed, try to format FS");
      if(SPIFFS.format()) {
        Serial.println("SPIFFS format success");
      }
      return;
    }
    WiFiClient *stream = http.getStreamPtr();
    const size_t bufferSize = 64;
    uint8_t buffer[bufferSize] = {0};
    size_t fileSize = http.getSize();
    size_t totalBytesRead = 0;
    size_t bytesRead = stream->readBytes(buffer, WAV_HEAD_SIZE); // WAV文件有固定44字节的头
    WRITE_TO_FILE(file, buffer, bytesRead);
    totalBytesRead += bytesRead;
    while (totalBytesRead < fileSize) {
      size_t bytesToRead = min(bufferSize, fileSize - totalBytesRead);
      size_t bytesRead = stream->readBytes(buffer, bytesToRead);
      if (bytesRead > 0) {
        if(play) { // 边下载边播放，可能因为网络原因出现卡顿
          write((const int16_t *)buffer, bytesRead);
        }
        WRITE_TO_FILE(file, buffer, bytesRead);
        totalBytesRead += bytesRead;
      } else {
        break;
      }
    }
    http.end();
    stop();
    if(file) {
      file.close();
    }
    Serial.printf("download success, file size is %d\n", fileSize);
  }

  bool play(const char *filePath) {
    Serial.printf("play with sniffs file: %s\n", filePath);
    File file = SPIFFS.open(filePath, FILE_READ);
    if (!file) {
      Serial.println("play error: file not found");
      return false;
    }
    size_t fileSize = file.size();
    Serial.printf("fileSize is %d\n", fileSize);
    if(fileSize < 44) {
      Serial.println("invalid audio file size, need re-download");
      if(SPIFFS.remove(filePath)) {
        Serial.printf("file:%s removed\r\n", filePath);
      }
      return false;
    }
    const size_t bufferSize = DMA_BUF_LEN;
    int16_t buffer[bufferSize];
    size_t totalBytesRead = 0;
    size_t bytesRead = file.read((uint8_t *)buffer, WAV_HEAD_SIZE); // 固定wav头
    totalBytesRead += bytesRead;
    stopFlag = false;
    while (totalBytesRead < fileSize && !stopFlag) {
      size_t bytesToRead = min(bufferSize, fileSize - totalBytesRead);
      bytesRead = file.read((uint8_t *)buffer, bytesToRead);
      if (bytesRead != bytesToRead) {
        Serial.println("ReadError");
        break;
      }
      write(buffer, bytesToRead);
      totalBytesRead += bytesRead;
    }
    stop();
    file.close();
    Serial.println("play ok");
    return true;
  }
};
