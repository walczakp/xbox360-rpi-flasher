#include "XSPI.h"
#include "XNAND.h"
#include "functions.h"
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <argp.h>

char desc[] = "XBOX360 NAND Flasher for Raspbery Pi";
struct argp argp = { NULL, NULL, NULL, desc };


uint8_t IN_FLASHMODE = 0;

void read(struct FlashConfig*, char*);
void cleanup(void);

int main(int argc, char *argv[]) {
    atexit(cleanup);

    argp_parse(&argp, argc, argv, 0, 0, NULL);
    // exit(0);

    printf("Initializing XBOX360 SPI...\n");
    XSPI_Init();

    printf("Entering flashmode...\n");
    XSPI_EnterFlashmode();
    IN_FLASHMODE = 1;

    printf("Reading flash config...\n");
    struct FlashConfig config;
    if (!read_flash_config(&config)) {
        printf("Your flash config is incorrect, check your wiring!\n");
        exit(1);
    }
//    print_flash_config(&config);
    

    read(&config, "nand1.bin");
    read(&config, "nand2.bin");

    exit(0);
}

void read(struct FlashConfig* config, char* outputFilename) {
    printf("file: %s\n", outputFilename);
    FILE *ofp;
    ofp = fopen(outputFilename, "wb");
    if (ofp == NULL) {
        fprintf(stderr, "Can't open output file %s!\n", outputFilename);
        perror("Error cause");
        exit(2);
    }
 
    uint32_t start = 0x00;
    uint32_t blocks = config->blocks_count;
    uint32_t rawPageSize = config->page_size + CRC_DATA_PER_PAGE;
    uint32_t rawBlockSize = rawPageSize * config->pages_count;
    uint32_t buff_size = sizeof(uint8_t) * blocks * rawBlockSize;
    uint8_t *buff = (uint8_t*) malloc(buff_size);
    read_nand(start, blocks, config, buff);

    fwrite(buff, buff_size, 1, ofp);
    fclose(ofp);

    free(buff);
}

void cleanup(void) {
    if (IN_FLASHMODE) {
        printf("Leaving flashmode!\n");
        XSPI_LeaveFlashmode();
        IN_FLASHMODE = 0;
    }
}
