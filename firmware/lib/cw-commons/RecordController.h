#pragma once
#include <Arduino.h>
#include <CWPreferences.h>
#include <SPIFFS.h>
#include <driver/i2s.h>

// 麦克风
#define I2S_SD  34
#define I2S_WS  21
#define I2S_SCK 22
#define RECORD_DMA_BUF_LEN 1024
#define MIC_I2S_PORT I2S_NUM_1     // 因为驱动屏幕已经占用了I2S_NUM_1通道，且扬声器占用了I2S_NUM_0通道。已经没有更多I2S通道可用了，导致这里使用I2S_NUM_1是不能正常工作的
#define RECORD_I2S_SAMPLE_RATE   16000
#define WAV_FILE "/record1.wav"
#define WAV_HEADER_SIZE   44

volatile bool recording = false;

struct RecordController {
  static RecordController *getInstance() {
    static RecordController base;
    return &base;
  }
  
  bool micInit() {
    const i2s_config_t i2s_config = {
      .mode = i2s_mode_t(I2S_MODE_RX),
      .sample_rate = RECORD_I2S_SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 4,
      .dma_buf_len = RECORD_DMA_BUF_LEN,
      .use_apll = false
    };
    const i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_SCK,
      .ws_io_num = I2S_WS,
      .data_out_num = -1,
      .data_in_num = I2S_SD
    };
    i2s_driver_install(MIC_I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(MIC_I2S_PORT, &pin_config);  
    return true;
    //i2s_start一般不需要手动执行，在i2s_driver_install成功后，外设会自动开始工作
    //esp_err_t status = i2s_start(MIC_I2S_PORT);
    //bool ret = status == ESP_OK;
    //Serial.println(ret ? "I2S mic init success" : "I2S audio init failed");
    //return ret;
  }

  void begin() {
    micInit();
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
    header[16] = 0x10;
    header[17] = 0x00;
    header[18] = 0x00;
    header[19] = 0x00;
    header[20] = 0x01;
    header[21] = 0x00;
    header[22] = 0x01;
    header[23] = 0x00;
    header[24] = 0x80;
    header[25] = 0x3E;
    header[26] = 0x00;
    header[27] = 0x00;
    header[28] = 0x00;
    header[29] = 0x7D;
    header[30] = 0x01;
    header[31] = 0x00;
    header[32] = 0x02;
    header[33] = 0x00;
    header[34] = 0x10;
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

  static void recordTask(void *param) {
    size_t audioSize = 0;    
    int16_t pcmBuffer[RECORD_DMA_BUF_LEN];
    File fp = SPIFFS.open(WAV_FILE, FILE_WRITE);
    if(!fp) {
      Serial.println("fail to create file");
      goto end;
    }

    Serial.println("===> recording...");
    fp.seek(WAV_HEADER_SIZE);
    while(recording) {
      size_t size = 0;
      esp_err_t ret = i2s_read(MIC_I2S_PORT, pcmBuffer, RECORD_DMA_BUF_LEN, &size, pdMS_TO_TICKS(5));
      if (ret != ESP_OK ) {
        if(ret == ESP_ERR_INVALID_STATE) {
          i2s_stop(MIC_I2S_PORT);
          i2s_start(MIC_I2S_PORT); // 尝试恢复
        } else {
          break;
        }
      } else if(size <= 0) {
        vTaskDelay(1);
      } else {
        fp.write((uint8_t *)pcmBuffer, size);
        audioSize += size;
      }
    }

    uint8_t header[WAV_HEADER_SIZE];
    buildWavHeader(header, audioSize);
    fp.seek(0);
    fp.write(header, WAV_HEADER_SIZE);
    fp.close();
    Serial.printf("record finished, audio size is %d\n", audioSize);
end:
    recording = false;
    vTaskDelete(NULL);
  }

  void stopRecord() {
    recording = false;
  }
  
  bool startRecord() {
    TaskHandle_t handle = NULL;    
    xTaskCreatePinnedToCore(
      recordTask,
      "recordTask", 
      10240,
      NULL,
      1, 
      &handle,
      0
    );

    recording = handle != NULL;
    return recording;
  }
};


