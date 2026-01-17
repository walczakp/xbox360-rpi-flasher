/**
 * @file xnand.c
 * @brief Xbox 360 NAND Controller Implementation
 */

#include "xnand.h"
#include "utils.h"
#include "xspi.h"
#include <unistd.h>

// Registers
#define REG_CONFIG 0x00
#define REG_STATUS 0x04
#define REG_CMD 0x08
#define REG_ADDR 0x0C
#define REG_DATA 0x10

// Commands
#define CMD_READ 0x03
#define CMD_WRITE 0x01
#define CMD_PAGE 0x55
#define CMD_BLOCK 0xAA
#define CMD_UNLOCK 0x55
#define CMD_CONFIRM 0xAA
#define CMD_EXEC_WR 0x04
#define CMD_EXEC_ER 0x05

// Internal state
static XNAND_Config current_config;
static int is_initialized = 0;

int XNAND_Init(void) {
  if (is_initialized)
    return 0;

  if (XSPI_Init() < 0)
    return -1;
  XSPI_EnterFlashMode();

  // Read initial config
  current_config = XNAND_GetConfig();
  is_initialized = 1;
  return 0;
}

void XNAND_ClearStatus(void) {
  uint32_t status = XSPI_ReadReg(REG_STATUS);
  XSPI_WriteReg(REG_STATUS, status);
}

int XNAND_WaitReady(uint32_t timeout) {
  do {
    uint32_t status = XSPI_ReadReg(REG_STATUS);
    if (!(status & 0x01)) { // Busy bit
      return 0;
    }
    usleep(100); // 100us delay to prevent CPU spin
  } while (timeout--);

  printf("[-] XNAND_WaitReady: Timeout waiting for NAND (status stuck busy)\n");
  return -1; // Timeout
}

XNAND_Config XNAND_GetConfig(void) {
  XNAND_Config conf = {0};
  conf.flash_config = XSPI_ReadReg(REG_CONFIG);

  conf.major_version = (conf.flash_config >> 17) & 3;
  conf.minor_version = (conf.flash_config >> 4) & 3;

  // Determine block size and NAND size based on flash config
  // Reference: PicoFlasher and Xbox 360 documentation
  //
  // Small Block (16KB erase block):
  //   - 16MB: Xenon, Zephyr, Falcon, Opus, Trinity, Corona (SPI)
  //
  // Big Block (128KB or 256KB erase block):
  //   - 16MB Big Block: Some Jasper 16MB (rare)
  //   - 256MB: Jasper 256MB
  //   - 512MB: Jasper 512MB

  if (conf.major_version >= 1) {
    // Big Block NAND detected
    if (conf.minor_version == 2) {
      // 256MB Big Block
      conf.erase_block_size = 0x20000; // 128KB
      conf.size_mb = 256;
    } else if (conf.minor_version == 3) {
      // 512MB Big Block
      conf.erase_block_size = 0x40000; // 256KB
      conf.size_mb = 512;
    } else {
      // 16MB Big Block (minor_version == 0 or 1 with major >= 1)
      conf.erase_block_size = 0x20000; // 128KB
      conf.size_mb = 16;
    }
  } else {
    // Small Block NAND (major_version == 0)
    conf.erase_block_size = 0x4000; // 16KB
    conf.size_mb = 16;
  }

  conf.sectors_per_block = conf.erase_block_size / 0x200; // 512-byte sectors

  return conf;
}

void XNAND_PrintConfig(XNAND_Config conf) {
  printf("Flash Config: 0x%08X\n", conf.flash_config);
  printf("Size: %d MB\n", conf.size_mb);
  printf("Block Size: 0x%X bytes (%d sectors)\n", conf.erase_block_size,
         conf.sectors_per_block);
}

int XNAND_ReadSector(uint32_t sector_index, uint8_t *buffer, uint8_t *spare) {
  XNAND_ClearStatus();

  XSPI_WriteReg(REG_ADDR, sector_index << 9); // LBA to byte offset
  XSPI_WriteReg(REG_CMD, CMD_READ);

  if (XNAND_WaitReady(0x1000) < 0)
    return -1;

  XSPI_WriteReg(REG_ADDR, 0); // Reset pointer

  // Read 512 bytes data (128 words)
  for (int i = 0; i < 128; i++) {
    XSPI_WriteReg(REG_CMD, 0x00); // Clock data
    uint32_t val = XSPI_ReadReg(REG_DATA);
    *(uint32_t *)&buffer[i * 4] = val;
  }

  // Read 16 bytes spare (4 words)
  for (int i = 0; i < 4; i++) {
    XSPI_WriteReg(REG_CMD, 0x00);
    uint32_t val = XSPI_ReadReg(REG_DATA);
    *(uint32_t *)&spare[i * 4] = val;
  }

  return 0;
}

int XNAND_EraseBlock(uint32_t sector_index) {
  XNAND_ClearStatus();

  // Enable write/erase in config register
  uint32_t cfg = XSPI_ReadReg(REG_CONFIG);
  XSPI_WriteReg(
      REG_CONFIG,
      cfg | 0x08); // Set bit 3? Or is it implicit? xbox360-rpi-flasher does it.

  XSPI_WriteReg(REG_ADDR, sector_index << 9);

  // Unlock sequence
  XSPI_WriteReg(REG_CMD, CMD_BLOCK);
  XSPI_WriteReg(REG_CMD, CMD_UNLOCK);
  XSPI_WriteReg(REG_CMD, CMD_EXEC_ER);

  if (XNAND_WaitReady(0x1000) < 0)
    return -2;

  return 0;
}

int XNAND_WriteSector(uint32_t sector_index, const uint8_t *buffer,
                      const uint8_t *spare) {
  // Check start of block and erase if necessary
  // Note: This logic assumes sequential writing! Random access writing might
  // erase neighbors.
  if ((sector_index % current_config.sectors_per_block) == 0) {
    if (XNAND_EraseBlock(sector_index) < 0)
      return -2;
  }

  XNAND_ClearStatus();
  XSPI_WriteReg(REG_ADDR, 0);

  // Write 512 data
  for (int i = 0; i < 128; i++) {
    XSPI_WriteReg(REG_DATA, *(uint32_t *)&buffer[i * 4]);
    XSPI_WriteReg(REG_CMD, CMD_WRITE);
  }

  // Write 16 spare
  for (int i = 0; i < 4; i++) {
    XSPI_WriteReg(REG_DATA, *(uint32_t *)&spare[i * 4]);
    XSPI_WriteReg(REG_CMD, CMD_WRITE);
  }

  if (XNAND_WaitReady(0x1000) < 0)
    return -3;

  XSPI_WriteReg(REG_ADDR, sector_index << 9);

  if (XNAND_WaitReady(0x1000) < 0)
    return -4;

  // Execute Write sequence
  XSPI_WriteReg(REG_CMD, CMD_UNLOCK);
  XSPI_WriteReg(REG_CMD, CMD_CONFIRM);
  XSPI_WriteReg(REG_CMD, CMD_EXEC_WR);

  if (XNAND_WaitReady(0x1000) < 0)
    return -5;

  return 0;
}
