#include "dccrmt.h"

#include "Arduino.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_types.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// SemaphoreHandle_t mutex = xSemaphoreCreateMutex();

size_t DCCRMT::rmt_encode_dcc_packet(rmt_encoder_t* encoder, rmt_channel_handle_t channel, const void* primary_data,
                                     size_t data_size, rmt_encode_state_t* ret_state) {
  rmt_dcc_packet_encoder_t* packet_encoder = __containerof(encoder, rmt_dcc_packet_encoder_t, base);
  rmt_encoder_handle_t copy_encoder = packet_encoder->copy_encoder;
  rmt_encode_state_t session_state = RMT_ENCODING_RESET;
  rmt_symbol_word_t* packet = (rmt_symbol_word_t*)primary_data;

  size_t encoded_symbols = copy_encoder->encode(copy_encoder, channel, packet, sizeof(*packet), &session_state);
  *ret_state = session_state;
  return encoded_symbols;
}

esp_err_t DCCRMT::rmt_del_dcc_packet_encoder(rmt_encoder_t* encoder) {
  rmt_dcc_packet_encoder_t* packet_encoder = __containerof(encoder, rmt_dcc_packet_encoder_t, base);
  rmt_del_encoder(packet_encoder->copy_encoder);
  free(packet_encoder);
  return ESP_OK;
}

esp_err_t DCCRMT::rmt_dcc_packet_encoder_reset(rmt_encoder_t* encoder) {
  rmt_dcc_packet_encoder_t* packet_encoder = __containerof(encoder, rmt_dcc_packet_encoder_t, base);
  rmt_encoder_reset(packet_encoder->copy_encoder);
  return ESP_OK;
}

esp_err_t DCCRMT::rmt_new_dcc_packet_encoder(const dcc_packet_encoder_config_t* config, rmt_encoder_handle_t* ret_encoder) {
  rmt_dcc_packet_encoder_t* packet_encoder = NULL;
  packet_encoder = (rmt_dcc_packet_encoder_t*)calloc(1, sizeof(rmt_dcc_packet_encoder_t));
  packet_encoder->base.encode = rmt_encode_dcc_packet;
  packet_encoder->base.del = rmt_del_dcc_packet_encoder;
  packet_encoder->base.reset = rmt_dcc_packet_encoder_reset;
  packet_encoder->resolution = config->resolution;
  rmt_copy_encoder_config_t copy_encoder_config = {};
  rmt_new_copy_encoder(&copy_encoder_config, &packet_encoder->copy_encoder);
  *ret_encoder = &packet_encoder->base;
  return ESP_OK;
}

void DCCRMT::taskCallback(void* pvParameters) {
  while (1) {
    event_callback_struct_t* transmitParams = (event_callback_struct_t*)pvParameters;

    if (transmitParams->dccRmtPacket->useSymbols1) {
      for (int i = 0; i < transmitParams->dccRmtPacket->count1; i++) {
        rmt_transmit(transmitParams->tx_channel, transmitParams->dcc_encoder, &(transmitParams->dccRmtPacket->symbols1[i]),
                     sizeof(rmt_symbol_word_t), &transmitParams->tx_config);
      }
    } else {
      for (int i = 0; i < transmitParams->dccRmtPacket->count0; i++) {
        rmt_transmit(transmitParams->tx_channel, transmitParams->dcc_encoder, &(transmitParams->dccRmtPacket->symbols0[i]),
                     sizeof(rmt_symbol_word_t), &transmitParams->tx_config);
      }
    }
  }
}

void DCCRMT::setupRmt(gpio_num_t pin, bool isMain) {
  ESP_LOGD(TAG, "Create RMT TX channel");
  preambleBits = isMain ? DCC_PREAMBLE_BITS_MAIN : DCC_PREAMBLE_BITS_PROG;
  rmtTxData->tx_channel = NULL;
  rmt_tx_channel_config_t dcc_tx_chan_config = {
      .gpio_num = pin,
      .clk_src = RMT_CLK_SRC_DEFAULT,  // select source clock
      .resolution_hz = DCC_RMT_CLOCK_FREQUENCY,
      .mem_block_symbols = SOC_RMT_MEM_WORDS_PER_CHANNEL,
      .trans_queue_depth = (size_t)(isMain ? DCC_RMT_TX_QUEUE_DEPTH_MAIN : DCC_RMT_TX_QUEUE_DEPTH_PROG),
      .flags =
          {
              .invert_out = DCC_SIGNAL_OUT_INVERT,
              .with_dma = false,
              .io_loop_back = false,
              .io_od_mode = false,
          },
      .intr_priority = 2,

  };
  ESP_ERROR_CHECK(rmt_new_tx_channel(&dcc_tx_chan_config, (&rmtTxData->tx_channel)));
  rmtTxData->dcc_encoder = NULL;
  dcc_packet_encoder_config_t dcc_encoder_config = {.resolution = DCC_RMT_CLOCK_FREQUENCY};
  ESP_ERROR_CHECK(rmt_new_dcc_packet_encoder(&dcc_encoder_config, &(rmtTxData->dcc_encoder)));

  ESP_LOGD(TAG, "Enable RMT TX channel");
  ESP_ERROR_CHECK(rmt_enable(rmtTxData->tx_channel));
  ESP_LOGD(TAG, "Enabled ");
}

void DCCRMT::blitter(uint8_t* data, size_t items) {
  rmt_symbol_word_t* symbols;
  rmtTxData->dccRmtPacket->useSymbols1 ? symbols = rmtTxData->dccRmtPacket->symbols0 : symbols = rmtTxData->dccRmtPacket->symbols1;

  // Preamble
  int count = 0;
  for (int i = 0; i <= preambleBits - 1; i++) {
    symbols[count] = mark;
    count++;
  }

  // Start bit
  symbols[count] = space;
  count++;

  // This is where the magic happens.
  uint8_t errorByte = 0;
  for (int i = 0; i <= items; i++) {
    uint8_t loopByte = data[i];
    (i != items) ? errorByte = errorByte ^ loopByte : loopByte = errorByte;

    for (int8_t j = 8; j >= 0; j = j - 1) {
      // I have to go through this again. Forget exactly the logic, but it works.
      (((uint16_t)loopByte & 1 << (j - 1)) | (((j - 1) == -1) & (i == items))) ? symbols[count] = mark : symbols[count] = space;
      count++;
    }
  }

  rmtTxData->dccRmtPacket->useSymbols1 ? rmtTxData->dccRmtPacket->count0 = count : rmtTxData->dccRmtPacket->count1 = count;
  rmtTxData->dccRmtPacket->useSymbols1 = !(rmtTxData->dccRmtPacket->useSymbols1);

  return;
}

DCCRMT::DCCRMT(gpio_num_t pin, bool isMain) {
  ESP_LOGD(TAG, "DCC Task Enter");

  // Init class var
  rmtTxData = (event_callback_struct_t*)calloc(1, sizeof(event_callback_struct_t));
  rmtTxData->dccRmtPacket = (dcc_rmt_symbols_t*)calloc(1, sizeof(dcc_rmt_symbols_t));

  // Setup the RMT TX peripheral
  setupRmt(pin, isMain);

  // Create initial entry for RMT TX.
  isMain ? SendSimplePacket(0xFF, 0x00) : SendSimplePacket(0x00, 0x00);

  // Task Creation
  xTaskBuffer = (StaticTask_t)calloc(1, sizeof(StaticTask_t));
  xStack = (StackType_t*)calloc(1, sizeof(StackType_t) * 2048);
  xTaskCreateStaticPinnedToCore(&taskCallback, isMain ? DCC_IDLE_TIMER_NAME_MAIN : DCC_IDLE_TIMER_NAME_PROG, 2048, rmtTxData,
                                DCC_RMT_TX_TASK_PRI, xStack, &xTaskBuffer, DCC_RMT_TASK_PIN_CORE);
}

void DCCRMT::SendSimplePacket(uint8_t Address, uint8_t Command) {
  uint8_t dataArray[2] = {Address, Command};
  blitter(dataArray, 2);
  return;
}
