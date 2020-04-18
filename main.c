#include "XNAND.h"
#include "XSPI.h"
#include "unpack.h"
#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

uint8_t FlashConfig1[4];
uint8_t FlashConfig2[4];

uint32_t blockSize;
uint32_t blockCount;
uint32_t pageSize;
uint32_t pageCount;

void cleanup(void) {
  printf("Leaving flashmode!\n");
  XSPI_LeaveFlashmode(0);
}

void read_nand(uint32_t start, uint32_t blocks, uint8_t *buffer) {
  printf("Reading flash blocks 0x%04x-0x%04x...\n", start, blocks);
  uint32_t wordsLeft = 0;
  uint32_t nextPage = start;

  // Len is total number of 32 bit words (4 bytes) we need to get
  uint32_t len = blocks * (blockSize / 4); // block size + spares
  while (len) {
    uint8_t readNow;

    printf("Block: 0x%04x\r", nextPage / pageCount);

    if (!wordsLeft) {
      //Set Read Page
      XNAND_StartRead(nextPage);
      nextPage++;
      wordsLeft = pageSize / 4;
    }

    //How many words we are about to read
    readNow = (len < wordsLeft) ? len : wordsLeft;
    //Read Page to Buffer
    XNAND_ReadFillBuffer(buffer, readNow);

    //Iterate
    buffer += (readNow * 4);
    wordsLeft -= readNow;
    len -= readNow;
  }

  printf("\nRead 0x%04x/0x%04x blocks\n", nextPage / pageCount, blocks);
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
  // Calculate how big our buffer is in bytes.
  uint32_t buff_size = sizeof(uint8_t) * blockCount * blockSize;
  uint8_t *buff = (uint8_t *)malloc(buff_size);

  // Read nand into buffer
  read_nand(start, blockCount, buff);

  // Write buffer to file
  fwrite(buff, buff_size, 1, ofp);

  // Cleanup
  fclose(ofp);
  free(buff);
}

// Set nand block information based on the nand config identifier
// https://github.com/Free60Project/wiki/blob/master/NAND.md
void set_config(uint32_t config) {
  //Large Block: 256MB NAND
  if(0x008A3020 == config) {
    blockSize = 0x21000 ;
    //After block 1000 is used as Memory Unit
    //blockCount = 0x800;
    blockCount = 0x200;
    pageSize = 0x210;
    pageCount = 0x100;
    //64MB Total size / 65MB With Spares (OS)
    //256MB Total size / 264MB With Spares (OS+MU)
    return;
  }

  //Large Block: 256/512MB NAND
  if(0x00aa3020 == config) {
    blockSize = 0x21000 ;
    //After block 0x200 (64M) is used as Memory Unit
    //Actual total blocks is 0x1000
    blockCount = 0x200;
    blockCount = 0x200;//short test
    pageSize = 0x210;
    pageCount = 0x100;
    //64MB Total size / 65MB With Spares (OS)
    //512MB Total size / 528MB With Spares (OS+MU)
    return;
  }

  // Small Block: 16MB NAND
  if(0x00aaaaaa == config) {//TODO: Need Config ID
    blockSize = 0x4200;
    blockCount = 0x400;
    pageSize = 0x210;
    pageCount = 0x20;
    //16MB Total size / 16.5MB With Spares
    return;
  }

  // Small Block: 64MB NAND
  if(0x00aaaabb == config) {//TODO: Need Config ID
    blockSize = 0x4200;
    blockCount = 0x1000;
    pageSize = 0x210;
    pageCount = 0x20;
    //64MB Total size / 65MB With Spares
    return;
  }

  printf("No idea what chip this is.\n");
  exit(-1);
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

  set_config(flash_config2);

  nand_to_file("nand1.bin");
  nand_to_file("nand2.bin");

  exit(0);
}
