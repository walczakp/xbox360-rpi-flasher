/**
 * @file gpio.h
 * @brief GPIO abstraction layer for Raspberry Pi 4
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

// Pin Definitions (Broadcom GPIO numbers)
#define PIN_MOSI    10  // SPI MOSI (Master Out Slave In)
#define PIN_MISO    9   // SPI MISO (Master In Slave Out)
#define PIN_SCLK    11  // SPI Clock
#define PIN_SS      26  // SPI Chip Select (Software controlled)

#define PIN_XX      23  // Xbox Magic / SMC_RST_XDK_N (Active Low)
#define PIN_EJ      24  // Eject / SMC_DBG_EN (Active Low)

// Function Prototypes
int GPIO_Init(void);
void GPIO_Shutdown(void);
void GPIO_SetMode(int pin, int mode);
void GPIO_Write(int pin, int level);
int GPIO_Read(int pin);

#endif // GPIO_H
