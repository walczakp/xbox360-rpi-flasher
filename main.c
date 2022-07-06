#include "XNAND.h"
#include "XSPI.h"
#include "unpack.h"
#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

uint8_t FlashConfig1[4];
uint8_t FlashConfig2[4];

void read_nand(uint32_t start, uint32_t blocks, uint8_t *buffer) {
  printf("Reading flash blocks 0x%04x-0x%04x...\n", start, start+blocks-1);
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

void write_nand(uint32_t start, uint32_t blocks, uint8_t *buffer) {
  printf("Writing flash blocks 0x%04x-0x%04x...\n", start, start+blocks-1);
    uint32_t nextPage = start << 5;
        
    XNAND_Erase(nextPage);
    XNAND_StartWrite();

    uint32_t wordsLeft = 0;    
    uint32_t len = blocks * 0x4200 / 4;

    while(len)
    {
        uint8_t writeNow;

        if(!wordsLeft)
        {
            wordsLeft = 0x210 / 4;
        }
        
        writeNow = (len < wordsLeft) ? len : wordsLeft;

        XNAND_WriteProcess(buffer, writeNow);
        buffer += (writeNow*4);
        wordsLeft -= writeNow;
        len -= writeNow;
        
        //execute write if buffer in NAND controller is filled
        if(!wordsLeft)
        {
            XNAND_WriteExecute(nextPage++);
            XNAND_StartWrite();
        }
    }
}

void nand_to_file(char *outputFilename) {
  printf("file: %s\n", outputFilename);
  FILE *ofp;
  ofp = fopen(outputFilename, "wb");
  if (ofp == NULL) {
    fprintf(stderr, "Can't open output file %s!\n", outputFilename);
    perror("Error cause");
    XSPI_LeaveFlashmode(0);
    exit(2);
  }

  uint32_t start = 0x00;

  uint32_t blocks = 0x400; //retail or RGH image
  //uint32_t blocks = 0x050; //glitch.ecc image
  
  uint32_t buff_size = sizeof(uint8_t) * blocks * 0x4200;
  uint8_t *buff = (uint8_t *)malloc(buff_size);
  read_nand(start, blocks, buff);

  fwrite(buff, buff_size, 1, ofp);
  fclose(ofp);

  free(buff);
}

void file_to_nand(char *inputFilename) {
  printf("file: %s\n", inputFilename);
  FILE *ifp;
  ifp = fopen(inputFilename, "rb");
  if (ifp == NULL) {
    fprintf(stderr, "Can't open input file %s!\n", inputFilename);
    perror("Error cause");
    XSPI_LeaveFlashmode(0);
    exit(2);
  }

  fseek(ifp, 0L, SEEK_END);
  int size = ftell(ifp);
  rewind(ifp);
  printf("filesize: %i\n", size);
  uint32_t blocks = size / (sizeof(uint8_t) * 0x4200);
  
  for(int b=0; b<blocks; b++) {
    printf("Writing block: %i\n", b+1);
    uint32_t buff_size = sizeof(uint8_t) * 0x4200;
    uint8_t *buff = (uint8_t *)malloc(buff_size);
    fread(buff, buff_size, 1, ifp);
    write_nand(b, 1, buff);
    free(buff);
  }

  fclose(ifp);
}

int main(void) {
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
    XSPI_LeaveFlashmode(0);
    exit(1);
  }

  //comment out depending on what you want to do

  //read NAND: dump 3 times, check afterwards with cksum that they are identical!
  //nand_to_file("nand1.bin");
  //nand_to_file("nand2.bin");
  //nand_to_file("nand3.bin");

  //write NAND: write NAND and dump it again. compare input file and dump file with cksum to confirm that they are identical. When flashing less than 16MB (the full 0x400 blocks), make sure to adjust the blocks variable in nand_to_file() accordingly.
  //file_to_nand("updflash.bin");
  //nand_to_file("nand_dump.bin");

  XSPI_LeaveFlashmode(0);

  exit(0);
}
