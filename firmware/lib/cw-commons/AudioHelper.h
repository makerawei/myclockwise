#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <driver/i2s.h>

#define AUDOI_I2S_PORT I2S_NUM_0
#define WAV_HEAD_SIZE 44
#define DMA_BUF_LEN 1024
#define I2S_SAMPLE_RATE 24000
#define I2S_DOUT 2
#define I2S_BCLK 32
#define I2S_LRC 33

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
    if(!spiffsInited){
      Serial.println("SPIFFS init failed");
    } else {
      Serial.println("SPIFFS init success");
    }

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = false,

        .tx_desc_auto_clear = true,         // 自动清除DMA描述符
        .fixed_mclk = 0,                    // 不固定MCLK
        .mclk_multiple = I2S_MCLK_MULTIPLE_256,  // 主时钟倍频
        .bits_per_chan = I2S_BITS_PER_CHAN_16BIT // 每个通道的位宽
    };    
    esp_err_t i2s_install_status =
        i2s_driver_install(AUDOI_I2S_PORT, &i2s_config, 0, NULL);
    
    i2s_pin_config_t pin_config = {.bck_io_num = I2S_BCLK,
                                   .ws_io_num = I2S_LRC,
                                   .data_out_num = I2S_DOUT,
                                   .data_in_num = I2S_PIN_NO_CHANGE
    };
    pin_config.mck_io_num = I2S_PIN_NO_CHANGE;
    esp_err_t i2s_pin_status = i2s_set_pin(AUDOI_I2S_PORT, &pin_config);
    i2s_set_sample_rates(AUDOI_I2S_PORT, I2S_SAMPLE_RATE);

    bool ret = i2s_install_status == ESP_OK && i2s_pin_status == ESP_OK;
    if (ret) {
      Serial.println("I2S audio init success");
      isInited = true;
    } else {
      Serial.println("I2S audio init failed");
    }

    return ret;
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
    i2s_zero_dma_buffer(AUDOI_I2S_PORT);  // 清空DMA缓存
    const int zeroSamples = 1024;
    int32_t zeroData = 0;
    for (int i = 0; i < zeroSamples; i++) {
      size_t bytesWritten = 0;
      i2s_write(AUDOI_I2S_PORT, &zeroData, sizeof(zeroData), &bytesWritten,
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
    i2s_stop(AUDOI_I2S_PORT);
    // I2S0.conf.tx_fifo_reset = 1;
    // I2S0.conf.tx_fifo_reset = 0;
    i2s_zero_dma_buffer(AUDOI_I2S_PORT);
    i2s_start(AUDOI_I2S_PORT);
    vTaskDelay(pdMS_TO_TICKS(20));
  }

  void write(const int16_t *buffer, const size_t size) {
    for (size_t i = 0; i < size / sizeof(int16_t); i++) {
      size_t bytesWritten = 0;
      i2s_write(AUDOI_I2S_PORT, &buffer[i], sizeof(buffer[i]), &bytesWritten,
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
      10240,
      url != NULL ? (void *)url: NULL,
      1, 
      NULL,
      core
    );
  }
  
  void play(const int16_t *buffer, const size_t size) {}

  void play(String url) {
    char filePath[32] = {0};
    snprintf(filePath, sizeof(filePath), "/%d.wav", url_hash(url.c_str()));
    if(!play(filePath)) {
      download(url);
    }
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
      Serial.println("open cache file failed");
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
      vTaskDelay(pdTICKS_TO_MS(10));
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
    Serial.printf("play finished, audio file size is %d\n", fileSize);
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
      vTaskDelay(pdTICKS_TO_MS(10));
    }
    stop();
    file.close();
    Serial.println("play ok");
    return true;
  }
};
