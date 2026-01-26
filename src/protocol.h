/**
 * @file protocol.h
 * @brief JRunner USB CDC Protocol Parser
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// NAND Commands (from PicoFlasher)
#define CMD_GET_VERSION 0x00
#define CMD_GET_CONFIG 0x01
#define CMD_READ_FLASH 0x02
#define CMD_WRITE_FLASH 0x03
#define CMD_READ_STREAM 0x04

// eMMC Commands (Corona 4GB)
#define CMD_EMMC_DETECT 0x50
#define CMD_EMMC_INIT 0x51
#define CMD_EMMC_GET_CID 0x52
#define CMD_EMMC_GET_CSD 0x53
#define CMD_EMMC_GET_EXT_CSD 0x54
#define CMD_EMMC_READ 0x55
#define CMD_EMMC_READ_STREAM 0x56
#define CMD_EMMC_WRITE 0x57

// ISD1200 Voice IC Commands
#define CMD_ISD1200_INIT 0xA0

// System Commands
#define CMD_REBOOT_BOOTLOADER 0xFE

// Structure for basic command (1 byte cmd + 4 bytes arg)
// Packed to match wire format
#pragma pack(push, 1)
typedef struct {
  uint8_t cmd;
  uint32_t lba;
} Protocol_Cmd;
#pragma pack(pop)

/**
 * @brief Run the protocol server loop
 * @param fd File descriptor to read/write from (e.g. serial port or socket)
 */
void Protocol_Loop(int fd);

#endif // PROTOCOL_H
