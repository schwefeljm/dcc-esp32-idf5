#define defines_h
#pragma once

// TAG for ESP_LOGX statements
static const char* TAG = "DCC-RMT";

//////////////////////////////////////////////
//            DCC SETTINGS                  //
//////////////////////////////////////////////
//
// make calculations easy and set up for microseconds
#define DCC_RMT_CLOCK_FREQUENCY 1000000  // 1 / 1000000Hz = 1us
#define DCC_RMT_TX_QUEUE_DEPTH 1
#define DCC_1_HALFPERIOD 58   // 58us
#define DCC_0_HALFPERIOD 100  // 100us
#define DCC_SYMBOL_WORD_LEVEL0 LOW
#define DCC_SYMBOL_WORD_LEVEL1 HIGH

#define DCC_PREAMBLE_BITS_MAIN 16
#define DCC_PREAMBLE_BITS_PROG 22

#define DCC_RMT_TX_TASK_PRI 5

// NMRA DCC DCC Extended Packet Formats DRAFT (as of 2023-11-25) - S-9.2.1
// https://www.nmra.org/sites/default/files/standards/sandrp/pdf/s-9.2.1_dcc_extended_packet_formats.pdf
#define DCC_MAC_ERROR_CONTROL_BYTES 1
#define DCC_MAX_ADDRESS_BYTES 2      // NMRA standard extended packets,
#define DCC_MAX_INSTRUCTION_BYTES 3  // payload size WITHOUT checksum.
#define DCC_MAX_PACKET_SIZE DCC_MAX_ADDRESS_BYTES + DCC_MAX_INSTRUCTION_BYTES
#define DCC_MAX_RMT_SYMBOLS                                                                                 \
  (DCC_PREAMBLE_BITS_MAIN > DCC_PREAMBLE_BITS_PROG ? DCC_PREAMBLE_BITS_MAIN : DCC_PREAMBLE_BITS_PROG) + 1 + \
      (DCC_MAX_PACKET_SIZE * 9) + (DCC_MAC_ERROR_CONTROL_BYTES * 9)

#define DCC_IDLE_TIMER_NAME_MAIN "DCC Packet Timer - Main "
#define DCC_IDLE_TIMER_PERIOD_MAIN 4
#define DCC_IDLE_TIMER_QUEUE_DEPTH_MAIN 512  // 1024 = ~0.125s delay
#define DCC_IDLE_TIMER_NAME_PROG "DCC Packet Timer - Prog "
#define DCC_IDLE_TIMER_PERIOD_PROG 4
#define DCC_IDLE_TIMER_QUEUE_DEPTH_PROG 1024  // 1024 = ~0.125s delay

#define DCC_MSG_QUEUE_DEPTH_MAIN 50
#define DCC_MSG_QUEUE_DEPTH_PROG 256

// TODO: Need to figure out how to calc these properly.
#define DCC_RMT_STMBOL_TIMING_OFFSET_0 13
#define DCC_RMT_STMBOL_TIMING_OFFSET_1 22

#define DCC_SIGNAL_OUT_INVERT true
