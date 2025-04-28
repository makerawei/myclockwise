#pragma once

#include <Arduino.h>
#include <SPIFFS.h>
#include <driver/i2s.h>
#include <HTTPClient.h>


#define AUDOI_I2S_PORT  I2S_NUM_0
#define DMA_BUF_LEN     1024
#define I2S_SAMPLE_RATE 24000
#define I2S_DOUT        2
#define I2S_BCLK        32
#define I2S_LRC         33

static bool isInited = false;


struct AudioHelper 
{
	static AudioHelper *getInstance()
	{
		static AudioHelper base;
		return &base;
	}


  bool begin()
  {
    if(isInited) {
      return true;
    }
    
    i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = I2S_SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // Change to ONLY_LEFT
      .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = 0,
      .dma_buf_count = 2,
      .dma_buf_len = DMA_BUF_LEN,
      .use_apll = false
    };
    esp_err_t i2s_install_status = i2s_driver_install(AUDOI_I2S_PORT, &i2s_config, 0, NULL);

    i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_BCLK,
      .ws_io_num = I2S_LRC,
      .data_out_num = I2S_DOUT,
      .data_in_num = I2S_PIN_NO_CHANGE
    };
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

  void stop() {
    const int zeroSamples = 1024;
    int32_t zeroData = 0;
    for (int i = 0; i < zeroSamples; i++) {
      size_t bytesWritten = 0;
      i2s_write(AUDOI_I2S_PORT, &zeroData, sizeof(zeroData), &bytesWritten, portMAX_DELAY);
    }
    Serial.println("audio stopped");
  }

  void write(const int16_t *buffer, const size_t size) {
    for (size_t i = 0; i < size / sizeof(int16_t); i++) {
      size_t bytesWritten = 0;
      i2s_write(AUDOI_I2S_PORT, &buffer[i], sizeof(buffer[i]), &bytesWritten, portMAX_DELAY);
    }
  }

  void jump() {
    String url = "http://makerawei-1251006064.cos.ap-guangzhou.myqcloud.com/clockwise/marioJump.wav";
    play(url);
  }

  void play(const int16_t *buffer, const size_t size) {
    
  }

  void play(String url) {
    if (WiFi.status() != WL_CONNECTED) {
      return;
    }
    Serial.println("wifi connected, start to download audio...");
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
      Serial.printf("invalid httpCode:%d\n", httpCode);
      return;
    }
    WiFiClient* stream = http.getStreamPtr();
    const size_t bufferSize = DMA_BUF_LEN;
    uint8_t buffer[bufferSize] = {0};
    size_t fileSize = http.getSize();
    size_t totalBytesRead = 0;
    size_t bytesRead = stream->readBytes(buffer, 44);
    totalBytesRead += bytesRead;
    
    while (totalBytesRead < fileSize) {
      size_t bytesToRead = min(bufferSize, fileSize - totalBytesRead);
      size_t bytesRead = stream->readBytes(buffer, bytesToRead);
      if(bytesRead > 0) {
        write((const int16_t *)buffer, bytesRead);
        totalBytesRead += bytesRead;
      } else {
        break;
      }
    }
    http.end();
    Serial.printf("play finished, audio file size is %d\n", fileSize);
  }
  
  void play(const char *filePath) {
    File file = SPIFFS.open(filePath, FILE_READ);
    if(!file){
      Serial.println("File not found");
      return;
    }
    size_t fileSize = file.size();
    const size_t bufferSize = DMA_BUF_LEN;
    int16_t buffer[bufferSize];
    size_t totalBytesRead = 0;
    size_t bytesRead = file.read((uint8_t *)buffer, 44); // 固定wav头
    totalBytesRead += bytesRead;
    while(totalBytesRead < fileSize) {
      size_t bytesToRead = min(bufferSize, fileSize - totalBytesRead);
      bytesRead = file.read((uint8_t *)buffer, bytesToRead);
      if(bytesRead != bytesToRead) {
        Serial.println("ReadError");
        break;
      }
      write(buffer, bytesToRead);
      totalBytesRead += bytesRead;
    }
    stop();
    file.close();
  }
  
};

