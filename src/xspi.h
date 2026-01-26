/**
 * @file xspi.h
 * @brief Xbox 360 SPI Protocol Implementation
 */

#ifndef XSPI_H
#define XSPI_H

#include "gpio.h"
#include <stdint.h>

// Initialize SPI interface and pins
int XSPI_Init(void);

// Shutdown SPI
void XSPI_Shutdown(void);

// Enter Flash Mode (handshake sequence)
void XSPI_EnterFlashMode(void);

// Leave Flash Mode
void XSPI_LeaveFlashMode(void);

// Read 32-bit register (auto-handles valid/status checks internally if needed)
uint32_t XSPI_ReadReg(uint8_t reg);

// Write 32-bit register
void XSPI_WriteReg(uint8_t reg, uint32_t val);

#endif // XSPI_H
