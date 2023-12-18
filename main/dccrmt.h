#define dccrmt_h
#pragma once
#ifdef __INTELLISENSE__
// 2906 - duplicate designator is not allowed
#pragma diag_suppress 2906
#endif

#include "defines.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// extern dcc_rmt_symbols_t* dccRmtPacket;
// extern SemaphoreHandle_t mutex;

class DCCRMT {
 private:
  typedef struct {
    volatile bool useSymbols1;
    uint8_t count0;
    uint8_t count1;
    rmt_symbol_word_t symbols0[DCC_MAX_RMT_SYMBOLS];
    rmt_symbol_word_t symbols1[DCC_MAX_RMT_SYMBOLS];
  } dcc_rmt_symbols_t;

  // Mark is a "1"
  const rmt_symbol_word_t mark = {
      .duration0 = DCC_1_HALFPERIOD - DCC_RMT_STMBOL_TIMING_OFFSET_1,
      .level0 = 0,
      .duration1 = DCC_1_HALFPERIOD,
      .level1 = 1,
  };

  // Space is a "0"
  const rmt_symbol_word_t space = {
      .duration0 = DCC_0_HALFPERIOD - DCC_RMT_STMBOL_TIMING_OFFSET_0,
      .level0 = 0,
      .duration1 = DCC_0_HALFPERIOD,
      .level1 = 1,
  };

  typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t* copy_encoder;
    uint32_t resolution;
  } rmt_dcc_packet_encoder_t;

  typedef struct {
    uint32_t resolution; /*!< Encoder resolution, in Hz */
  } dcc_packet_encoder_config_t;

  typedef struct {
    uint8_t items;
    uint8_t data[DCC_MAX_PACKET_SIZE];
  } dcc_data_t;

  typedef struct {
    rmt_channel_handle_t tx_channel;
    rmt_encoder_handle_t dcc_encoder;
    rmt_transmit_config_t tx_config;
    dcc_rmt_symbols_t* dccRmtPacket;
  } event_callback_struct_t;

  uint8_t preambleBits;
  StaticTask_t xTaskBuffer;
  StackType_t* xStack;
  event_callback_struct_t* rmtTxData;

  static size_t rmt_encode_dcc_packet(rmt_encoder_t* encoder, rmt_channel_handle_t channel, const void* primary_data,
                                      size_t data_size, rmt_encode_state_t* ret_state);
  static esp_err_t rmt_del_dcc_packet_encoder(rmt_encoder_t* encoder);
  static esp_err_t rmt_dcc_packet_encoder_reset(rmt_encoder_t* encoder);
  esp_err_t rmt_new_dcc_packet_encoder(const dcc_packet_encoder_config_t* config, rmt_encoder_handle_t* ret_encoder);
  static void taskCallback(void* pvParameters);
  void setupRmt(gpio_num_t pin, bool isMain);
  void blitter(uint8_t* data, size_t count);

 public:
  DCCRMT(gpio_num_t Pin, bool isMain);
  void SendSimplePacket(uint8_t Address, uint8_t Command);
};
