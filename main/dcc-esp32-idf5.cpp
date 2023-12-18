#include "Arduino.h"
#include "dccrmt.h"

extern "C" void app_main() {
  initArduino();
  DCCRMT rmt = DCCRMT(GPIO_NUM_1, true);
  while (1) {

    vTaskDelay(500);
    // By default, IDLE is sent on Main.
    // Use SendSimplePacket to send DCC command.
    // Format: SendSimplePacket(uint8_t Address, uint8_t Command)
    // Preamble, stat/stop bits and checksum is automatically calculated by blitter function
    rmt.SendSimplePacket(0x00, 0x00);
  }
}