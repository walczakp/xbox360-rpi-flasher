/**
 * @file xspi.c
 * @brief Xbox 360 SPI Protocol Implementation using pigpio hardware SPI
 */

#include "xspi.h"
#include "utils.h"
#include <string.h>

static int spi_handle = -1;

// SPI Configuration
#define SPI_CHANNEL 0
#define SPI_BAUD 10000000 // 10MHz - verified reliable (10x faster than original)

int XSPI_Init(void) {
  if (spi_handle >= 0)
    return 0; // Already initialized

  // Configure Control Pins
  GPIO_SetMode(PIN_XX, PI_OUTPUT);
  GPIO_SetMode(PIN_EJ, PI_OUTPUT);
  GPIO_SetMode(PIN_SS, PI_OUTPUT);

  // Initial State
  GPIO_Write(PIN_XX, 1);
  GPIO_Write(PIN_EJ, 1);
  GPIO_Write(PIN_SS, 1);

  // Initialize Hardware SPI
  // pigpio spiOpen: channel 0, baud, flags=0 (Mode 0)
  spi_handle = spiOpen(SPI_CHANNEL, SPI_BAUD, 0);
  if (spi_handle < 0) {
    printf("Failed to open SPI\n");
    return -1;
  }

  return 0;
}

void XSPI_Shutdown(void) {
  if (spi_handle >= 0) {
    spiClose(spi_handle);
    spi_handle = -1;
  }

  // Reset pins to safe state
  GPIO_Write(PIN_XX, 1);
  GPIO_Write(PIN_EJ, 1);
  GPIO_Write(PIN_SS, 1);
}

void XSPI_EnterFlashMode(void) {
  printf("[*] Entering Flash Mode...\n");

  GPIO_Write(PIN_XX, 0);
  gpioDelay(50000); // 50ms setup time

  GPIO_Write(PIN_SS, 0);
  GPIO_Write(PIN_EJ, 0);
  gpioDelay(50000);

  GPIO_Write(PIN_XX, 1);
  GPIO_Write(PIN_EJ, 1);
  gpioDelay(50000);

  GPIO_Write(PIN_SS, 0);
  printf("[*] Flash Mode ready. Reading flash config...\n");
}

void XSPI_LeaveFlashMode(void) {
  GPIO_Write(PIN_SS, 1);
  GPIO_Write(PIN_EJ, 0);
  gpioDelay(50000);

  GPIO_Write(PIN_XX, 0);
  GPIO_Write(PIN_EJ, 1);

  gpioDelay(50000);
  GPIO_Write(PIN_XX, 1); // Reset to idle
}

uint32_t XSPI_ReadReg(uint8_t reg) {
  // Cmd: [reg<<2 | 1] [0xFF] [0x00]*4
  // Total 6 bytes transaction
  uint8_t tx[6];
  uint8_t rx[6];

  tx[0] = (reg << 2) | 1;
  tx[1] = 0xFF; // Dummy/Turnaround
  tx[2] = 0x00;
  tx[3] = 0x00;
  tx[4] = 0x00;
  tx[5] = 0x00;

  // Reverse command byte (LSB first protocol)
  tx[0] = reverse_byte(tx[0]);
  // Other bytes are dummy or 0, reversal doesn't change 0xFF or 0x00

  GPIO_Write(PIN_SS, 0);
  // Small delay might be needed
  gpioDelay(2);

  spiXfer(spi_handle, (char *)tx, (char *)rx, 6);

  GPIO_Write(PIN_SS, 1);

  // Received data is in rx[2..5]
  // Bits are MSB-first from SPI hardware, need to reverse back to LSB-first
  uint8_t b0 = reverse_byte(rx[2]); // LSB
  uint8_t b1 = reverse_byte(rx[3]);
  uint8_t b2 = reverse_byte(rx[4]);
  uint8_t b3 = reverse_byte(rx[5]); // MSB

  // Combine into uint32_t (Little Endian)
  return (uint32_t)b0 | ((uint32_t)b1 << 8) | ((uint32_t)b2 << 16) |
         ((uint32_t)b3 << 24);
}

void XSPI_WriteReg(uint8_t reg, uint32_t val) {
  // Cmd: [reg<<2 | 2] [b0] [b1] [b2] [b3]
  uint8_t tx[5];
  uint8_t rx[5];

  tx[0] = (reg << 2) | 2;
  tx[1] = val & 0xFF;
  tx[2] = (val >> 8) & 0xFF;
  tx[3] = (val >> 16) & 0xFF;
  tx[4] = (val >> 24) & 0xFF;

  // Reverse all bytes for LSB-first transmission
  reverse_buffer(tx, 5);

  GPIO_Write(PIN_SS, 0);
  gpioDelay(2);

  spiXfer(spi_handle, (char *)tx, (char *)rx, 5);

  GPIO_Write(PIN_SS, 1);
}
