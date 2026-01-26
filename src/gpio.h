/**
 * @file gpio.h
 * @brief GPIO abstraction layer for Raspberry Pi
 */

#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Check if we are compiling on a non-Pi system (e.g., macOS for dev)
#ifdef __APPLE__
// Mock pigpio for development on Mac
#define PI_OUTPUT 1
#define PI_INPUT 0
#define PI_PUD_UP 2
#define PI_PUD_OFF 0
#define PI_PUD_DOWN 1

// Mock functions
int gpioInitialise(void);
void gpioTerminate(void);
int gpioSetMode(unsigned gpio, unsigned mode);
int gpioSetPullUpDown(unsigned gpio, unsigned pud);
int gpioWrite(unsigned gpio, unsigned level);
int gpioRead(unsigned gpio);
int gpioDelay(uint32_t micros);
int spiOpen(unsigned spiChan, unsigned baud, unsigned spiFlags);
int spiClose(unsigned handle);
int spiRead(unsigned handle, char *buf, unsigned count);
int spiWrite(unsigned handle, char *buf, unsigned count);
int spiXfer(unsigned handle, char *txBuf, char *rxBuf, unsigned count);
#else
#include <pigpio.h>
#endif

// Pi Model Types
typedef enum {
  PI_MODEL_4, // Raspberry Pi 4 - uses GPIO 26 for SS
  PI_MODEL_1B // Raspberry Pi 1 Model B - uses GPIO 8 for SS (26-pin header)
} PiModel;

// Pin Definitions (Broadcom GPIO numbers)
#define PIN_MOSI 10 // SPI MOSI (Master Out Slave In)
#define PIN_MISO 9  // SPI MISO (Master In Slave Out)
#define PIN_SCLK 11 // SPI Clock

// SS pin is variable based on Pi model
extern int PIN_SS; // SPI Chip Select (set by GPIO_SetPiModel)

#define PIN_XX 23 // Xbox Magic / SMC_RST_XDK_N (Active Low)
#define PIN_EJ 24 // Eject / SMC_DBG_EN (Active Low)

// Pi 4 uses GPIO 26 for SS
#define PIN_SS_PI4 26
// Pi 1B uses GPIO 8 (CE0) for SS (GPIO 26 not on 26-pin header)
#define PIN_SS_PI1B 8

// Function Prototypes
void GPIO_SetPiModel(PiModel model);
PiModel GPIO_GetPiModel(void);
int GPIO_Init(void);
void GPIO_Shutdown(void);
void GPIO_SetMode(int pin, int mode);
void GPIO_Write(int pin, int level);
int GPIO_Read(int pin);

#endif // GPIO_H
