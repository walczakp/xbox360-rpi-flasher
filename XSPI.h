#ifndef _XSPI_H_
#define _XSPI_H_

#include <inttypes.h>
#include <wiringPi.h>

#define EJ		24
#define	XX		23
#define	SS		8
#define SCK		11
#define MOSI		10
#define MISO		9

#define PINOUT(PIN)	pinMode(PIN, OUTPUT)
#define PININ(PIN)	pinMode(PIN, INPUT)

#define PINHIGH(PIN)	digitalWrite(PIN, HIGH)
#define PINLOW(PIN)	digitalWrite(PIN, LOW)

#define	PINGET(PIN)	digitalRead(PIN)

#define _delay_ms(MS)	delay(MS)


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
