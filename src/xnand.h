/**
 * @file xnand.h
 * @brief Xbox 360 NAND Controller Operations
 */

#ifndef XNAND_H
#define XNAND_H

#include <stdint.h>
#include <stdio.h>

// NAND Config structure
typedef struct {
  uint32_t flash_config;
  int major_version;
  int minor_version;
  int erase_block_size; // In bytes (0x4000, 0x20000, 0x40000)
  int sectors_per_block;
  int size_mb;
} XNAND_Config;

// Initialize NAND controller
int XNAND_Init(void);

// Get current Flash Config and decode it
XNAND_Config XNAND_GetConfig(void);

// Print config details to stdout
void XNAND_PrintConfig(XNAND_Config conf);

// Read a 528-byte sector (512 data + 16 spare)
// buffer must be at least 512 bytes
// spare must be at least 16 bytes
// Returns 0 on success, error code otherwise
int XNAND_ReadSector(uint32_t sector_index, uint8_t *buffer, uint8_t *spare);

// Write a 528-byte sector
// Auto-erases block if sector_index is at start of a block
int XNAND_WriteSector(uint32_t sector_index, const uint8_t *buffer,
                      const uint8_t *spare);

// Explicitly erase a block (by sector index)
int XNAND_EraseBlock(uint32_t sector_index);

#endif // XNAND_H
