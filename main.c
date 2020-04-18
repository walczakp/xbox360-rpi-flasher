#include "XNAND.h"
#include "XSPI.h"
#include "unpack.h"
#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

uint8_t FlashConfig1[4];
uint8_t FlashConfig2[4];

void cleanup(void) {
  printf("Leaving flashmode!\n");
  XSPI_LeaveFlashmode(0);
}

void read_nand(uint32_t start, uint32_t blocks, uint8_t *buffer) {
  printf("Reading flash blocks 0x%04x-0x%04x...\n", start, blocks);
  uint32_t wordsLeft = 0;
  uint32_t nextPage = start << 5;

  uint32_t len = blocks * (0x4200 / 4); // block size + spares
  while (len) {
    uint8_t readNow;

    if ((nextPage >> 5) % 4 == 0) {
      printf("0x%x\r", nextPage >> 5);
    }

    if (!wordsLeft) {
      XNAND_StartRead(nextPage);
      nextPage++;
      wordsLeft = 0x84;
    }

    readNow = (len < wordsLeft) ? len : wordsLeft;
    XNAND_ReadFillBuffer(buffer, readNow);

    buffer += (readNow * 4);
    wordsLeft -= readNow;
    len -= readNow;
  }

  printf("\nRead 0x%04x/%04x blocks\n", nextPage >> 5, blocks);
}

void nand_to_file(char *outputFilename) {
  printf("file: %s\n", outputFilename);
  FILE *ofp;
  ofp = fopen(outputFilename, "wb");
  if (ofp == NULL) {
    fprintf(stderr, "Can't open output file %s!\n", outputFilename);
    perror("Error cause");
    exit(2);
  }

  uint32_t start = 0x00;
  uint32_t blocks = 0x400;
  uint32_t buff_size = sizeof(uint8_t) * blocks * 0x4200;
  uint8_t *buff = (uint8_t *)malloc(buff_size);
  read_nand(start, blocks, buff);

  fwrite(buff, buff_size, 1, ofp);
  fclose(ofp);

  free(buff);
}

int main(void) {
  atexit(cleanup);

  printf("Initializing XBOX360 SPI...\n");
  XSPI_Init();

  printf("Entering flashmode...\n");
  XSPI_EnterFlashmode();

  printf("Reading flash config...\n");
  uint32_t flash_config1;
  uint32_t flash_config2;

  XSPI_Read(0, FlashConfig1);
  flash_config1 = unpack_uint32_le(FlashConfig1);
  printf("Flash config1: 0x%08x\n", flash_config1);

  XSPI_Read(0, FlashConfig2);
  flash_config2 = unpack_uint32_le(FlashConfig2);
  printf("Flash config2: 0x%08x\n", flash_config2);

  if (flash_config2 <= 0) {
    printf("Your flash config is incorrect, check your wiring!\n");
    exit(1);
  }

  nand_to_file("nand1.bin");
  nand_to_file("nand2.bin");
  nand_to_file("nand3.bin");

  exit(0);
}
