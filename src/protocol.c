/**
 * @file protocol.c
 * @brief JRunner Protocol Implementation
 */

#include "protocol.h"
#include "xnand.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PICOFLASHER_VERSION 3
#define SECTOR_SIZE 528

// Helper to read exactly N bytes
static int read_exact(int fd, void *buf, size_t count) {
  size_t received = 0;
  while (received < count) {
    ssize_t r = read(fd, (uint8_t *)buf + received, count - received);
    if (r < 0) {
      perror("read error");
      return -1;
    }
    if (r == 0) {
      // EOF / Disconnected. Use -2 to indicate retry needed.
      return -2;
    }
    received += r;
  }
  return 0;
}

// Helper to write exactly N bytes
static int write_exact(int fd, const void *buf, size_t count) {
  size_t sent = 0;
  while (sent < count) {
    ssize_t r = write(fd, (const uint8_t *)buf + sent, count - sent);
    if (r < 0)
      return -1;
    sent += r;
  }
  return 0;
}

void Protocol_Loop(int fd) {
  Protocol_Cmd pkt;
  uint8_t buffer[SECTOR_SIZE]; // 512 + 16
  uint32_t resp;

  printf("[*] Protocol Server Loop Started\n");
  printf("[*] Ready. Connect JRunner now.\n");

  while (1) {
    // 1. Read Command (5 bytes)
    int r = read_exact(fd, &pkt, sizeof(pkt));
    if (r == -2) {
      // Wait for connection...
      usleep(100000); // 100ms
      continue;
    }
    if (r < 0) {
      printf("[-] Connection lost (error)\n");
      break;
    }

    switch (pkt.cmd) {
    case CMD_GET_VERSION:
      // Instant response, no HW init needed
      resp = PICOFLASHER_VERSION;
      write_exact(fd, &resp, 4);
      break;

    case CMD_EMMC_DETECT:
      // Return 0 = no eMMC found (we're NAND only for now)
      resp = 0;
      write_exact(fd, &resp, 4);
      break;

    case CMD_EMMC_INIT:
    case CMD_EMMC_GET_CID:
    case CMD_EMMC_GET_CSD:
    case CMD_EMMC_GET_EXT_CSD:
    case CMD_EMMC_READ:
    case CMD_EMMC_READ_STREAM:
    case CMD_EMMC_WRITE:
      // eMMC not supported yet, return error
      resp = 0xFFFFFFFF;
      write_exact(fd, &resp, 4);
      break;

    case CMD_ISD1200_INIT:
      // ISD1200 not supported, return error
      resp = 0xFFFFFFFF;
      write_exact(fd, &resp, 4);
      break;

    case CMD_GET_CONFIG: {
      if (XNAND_Init() < 0) {
        uint32_t err = 0;
        write_exact(fd, &err, 4);
        break;
      }
      XNAND_Config conf = XNAND_GetConfig();
      write_exact(fd, &conf.flash_config, 4);
      break;
    }

    case CMD_READ_FLASH: {
      if (XNAND_Init() < 0) {
        resp = 0x8000;
        write_exact(fd, &resp, 4);
        break;
      }
      uint8_t *data = buffer;
      uint8_t *spare = buffer + 512;

      int ret = XNAND_ReadSector(pkt.lba, data, spare);

      resp = (ret == 0) ? 0 : 0x8000;
      write_exact(fd, &resp, 4);

      if (ret == 0) {
        write_exact(fd, buffer, SECTOR_SIZE);
      }
      break;
    }

    case CMD_WRITE_FLASH: {
      // Read payload first
      if (read_exact(fd, buffer, SECTOR_SIZE) < 0)
        break;

      if (XNAND_Init() < 0) {
        resp = 0x8000;
        write_exact(fd, &resp, 4);
        break;
      }

      uint8_t *data = buffer;
      uint8_t *spare = buffer + 512;

      int ret = XNAND_WriteSector(pkt.lba, data, spare);

      resp = (ret == 0) ? 0 : 0x8000;
      write_exact(fd, &resp, 4);
      break;
    }

    case CMD_READ_STREAM: {
      if (XNAND_Init() < 0) {
        resp = 0x8000;
        write_exact(fd, &resp, 4);
        break;
      }

      uint32_t count = pkt.lba;

      for (uint32_t i = 0; i < count; i++) {
        uint8_t *data = buffer;
        uint8_t *spare = buffer + 512;

        int ret = XNAND_ReadSector(i, data, spare);
        resp = (ret == 0) ? 0 : 0x8000;

        if (write_exact(fd, &resp, 4) < 0)
          break;

        if (ret == 0) {
          if (write_exact(fd, buffer, SECTOR_SIZE) < 0)
            break;
        } else {
          break;
        }
      }
      break;
    }

    default:
      printf("[-] Unknown command 0x%02X\n", pkt.cmd);
      break;
    }
  }
}
