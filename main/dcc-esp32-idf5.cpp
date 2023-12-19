#include "Arduino.h"
#include "dccrmt.h"

extern "C" void app_main() {
  initArduino();
  DCCRMT rmt = DCCRMT(GPIO_NUM_1, true);
  pinMode(2, OUTPUT);
  digitalWrite(2, !digitalRead(2));
  ESP_LOGD(TAG, "Value: %i", DCC_RMT_STMBOL_TIMING_OFFSET_1);
  while (1) {

    vTaskDelay(500);
    // By default, IDLE is sent on Main.
    // Use SendSimplePacket to send DCC command.
    // Format: SendSimplePacket(uint8_t Address, uint8_t Command)
    // Preamble, stat/stop bits and checksum is automatically calculated by blitter function
    digitalWrite(2, !digitalRead(2));
    rmt.SendSimplePacket(0x00, 0x00);
    digitalWrite(2, !digitalRead(2));
  }
}