#pragma once
#include <Arduino.h>
#include <CWPreferences.h>
#include <driver/i2s.h>


#define I2S_PORT I2S_NUM_0
// 扬声器
#define I2S_DOUT 2
#define I2S_BCLK 32
#define I2S_LRC 33
// 麦克风
#define I2S_SD  34
#define I2S_WS  21
#define I2S_SCK 22

#define SPK_I2S_SAMPLE_RATE 24000
#define MIC_I2S_SAMPLE_RATE 16000
#define DMA_BUF_LEN 1024


struct I2SController {
  bool _txMode = true; // 默认音频播放模式
  SemaphoreHandle_t i2s_switch_sema = xSemaphoreCreateBinary();
  // 用于扬声器
  const i2s_config_t i2s_config_tx = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SPK_I2S_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = DMA_BUF_LEN,
    .use_apll = false/*,
    .tx_desc_auto_clear = true,         // 自动清除DMA描述符
    .fixed_mclk = 0,                    // 不固定MCLK
    .mclk_multiple = I2S_MCLK_MULTIPLE_256,  // 主时钟倍频
    .bits_per_chan = I2S_BITS_PER_CHAN_16BIT // 每个通道的位宽 */
  };
    
  i2s_pin_config_t pin_config_tx = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  // 用于麦克风  
  i2s_config_t i2s_config_rx = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = MIC_I2S_SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = DMA_BUF_LEN,
    .use_apll = false
  };
  i2s_pin_config_t pin_config_rx = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };


  I2SController() {
    i2s_driver_install(I2S_PORT, &i2s_config_tx, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config_tx);
    xSemaphoreGive(i2s_switch_sema);
  }
  
  static I2SController *getInstance() {
    static I2SController base;
    return &base;
  }

  void switchLock() {
    xSemaphoreTake(i2s_switch_sema, portMAX_DELAY);
  }

  void switchUnlock() {
    xSemaphoreGive(i2s_switch_sema);
  }

  
  // 音频播放模式
  void switchToTx() {
    Serial.println("switch I2S to TX mode");
    if(isTxMode()) {
      return;
    }
    xSemaphoreTake(i2s_switch_sema, portMAX_DELAY);
    i2s_driver_uninstall(I2S_PORT);
    i2s_driver_install(I2S_PORT, &i2s_config_tx, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config_tx);
    i2s_set_sample_rates(I2S_PORT, SPK_I2S_SAMPLE_RATE);
    _txMode = true;
    vTaskDelay(pdMS_TO_TICKS(10));
    xSemaphoreGive(i2s_switch_sema);
  }

  // 录音模式
  void switchToRx() {
    Serial.println("switch I2S to RX mode");
    if(!isTxMode()) {
      return;
    }
    xSemaphoreTake(i2s_switch_sema, portMAX_DELAY);
    i2s_driver_uninstall(I2S_PORT);
    i2s_driver_install(I2S_PORT, &i2s_config_rx, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config_rx);
    _txMode = false;
    vTaskDelay(pdMS_TO_TICKS(10));
    xSemaphoreGive(i2s_switch_sema);
  }

  bool isTxMode() {
    return _txMode;
  }
};

