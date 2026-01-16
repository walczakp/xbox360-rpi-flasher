/**
 * @file gpio.c
 * @brief GPIO implementation using pigpio
 */

#include "gpio.h"

#ifdef __APPLE__
// Mock implementation for macOS development
int gpioInitialise(void) {
  printf("[MOCK] gpioInitialise\n");
  return 0;
}
void gpioTerminate(void) { printf("[MOCK] gpioTerminate\n"); }
int gpioSetMode(unsigned gpio, unsigned mode) {
  (void)gpio;
  (void)mode;
  return 0;
}
int gpioSetPullUpDown(unsigned gpio, unsigned pud) {
  (void)gpio;
  (void)pud;
  return 0;
}
int gpioWrite(unsigned gpio, unsigned level) {
  (void)gpio;
  (void)level;
  return 0;
}
int gpioRead(unsigned gpio) {
  (void)gpio;
  return 0;
}
int gpioDelay(uint32_t micros) {
  (void)micros;
  return 0;
}
int spiOpen(unsigned spiChan, unsigned baud, unsigned spiFlags) {
  (void)spiFlags;
  printf("[MOCK] spiOpen channel %d at %d baud\n", spiChan, baud);
  return 0;
}
int spiClose(unsigned handle) {
  (void)handle;
  return 0;
}
int spiRead(unsigned handle, char *buf, unsigned count) {
  (void)handle;
  (void)buf;
  return count;
}
int spiWrite(unsigned handle, char *buf, unsigned count) {
  (void)handle;
  (void)buf;
  return count;
}
int spiXfer(unsigned handle, char *txBuf, char *rxBuf, unsigned count) {
  (void)handle;
  (void)txBuf;
  (void)rxBuf;
  return count;
}
#endif

int GPIO_Init(void) {
  if (gpioInitialise() < 0) {
    fprintf(stderr, "pigpio initialization failed\n");
    return -1;
  }
  return 0;
}

void GPIO_Shutdown(void) { gpioTerminate(); }

void GPIO_SetMode(int pin, int mode) { gpioSetMode(pin, mode); }

void GPIO_Write(int pin, int level) { gpioWrite(pin, level); }

int GPIO_Read(int pin) { return gpioRead(pin); }
