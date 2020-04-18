#ifndef _XSPI_H_
#define _XSPI_H_

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <pigpio.h>

#define EJ 24   // Eject
#define XX 23   // Xbox Magic
#define SS 26   // Chip Select
#define SCK 11  // Serial Clock
#define MOSI 10 // Master out Slave in
#define MISO 9  // Master in Slave out

#define PINOUT(PIN) gpioSetMode(PIN, PI_OUTPUT)
#define PININ(PIN)                 \
gpioSetPullUpDown(PIN, PI_PUD_UP); \
gpioSetMode(PIN, PI_INPUT)

#define PINHIGH(PIN) gpioWrite(PIN, 1)
#define PINLOW(PIN) gpioWrite(PIN, 0)

#define PINGET(PIN) gpioRead(PIN)

#define _delay_ms(MS) delay(MS)

void XSPI_Init(void);

void XSPI_Powerup(void);
void XSPI_Shutdown(void);

void XSPI_EnterFlashmode(void);
void XSPI_LeaveFlashmode(uint8_t force);

void XSPI_Read(uint8_t reg, uint8_t *buf);
uint16_t XSPI_ReadWord(uint8_t reg);
uint8_t XSPI_ReadByte(uint8_t reg);

void XSPI_Write(uint8_t reg, uint8_t *buf);
void XSPI_WriteByte(uint8_t reg, uint8_t byte);
void XSPI_WriteDword(uint8_t reg, uint32_t dword);
void XSPI_Write0(uint8_t reg);

uint8_t XSPI_FetchByte(void);
void XSPI_PutByte(uint8_t out);

#endif
