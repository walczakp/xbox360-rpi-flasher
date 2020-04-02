#ifndef _XSPI_H_
#define _XSPI_H_

#include <inttypes.h>
#include <pigpio.h>

#define EJ		24
#define	XX		23
#define	SS		8
#define SCK		11
#define MOSI		10
#define MISO		9

#define PINOUT(PIN)	gpioSetMode(PIN, PI_OUTPUT)
#define PININ(PIN)	gpioSetMode(PIN, PI_INPUT); gpioSetPullUpDown(PIN, PI_PUD_DOWN)

#define PINHIGH(PIN) gpioWrite(PIN, 1)
#define PINLOW(PIN)	gpioWrite(PIN, 0)

#define	PINGET(PIN)	gpioRead(PIN)

#define _delay_ms(MS)	gpioSleep(PI_TIME_RELATIVE, 0, MS * 1000)


void XSPI_Init(void);

void XSPI_Powerup(void);
void XSPI_Shutdown(void);

void XSPI_EnterFlashmode(void);
void XSPI_LeaveFlashmode(uint8_t force);

void XSPI_Read(uint8_t reg, uint8_t* buf);
uint16_t XSPI_ReadWord(uint8_t reg);
uint8_t XSPI_ReadByte(uint8_t reg);

void XSPI_Write(uint8_t reg, uint8_t* buf);
void XSPI_WriteByte(uint8_t reg, uint8_t byte);
void XSPI_WriteDword(uint8_t reg, uint32_t dword);
void XSPI_Write0(uint8_t reg);

uint8_t XSPI_FetchByte(void);
void XSPI_PutByte(uint8_t out);

#endif
