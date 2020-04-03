#include "functions.h"
#include <stdio.h>
#include <wiringPi.h>
#include "XNAND.h"
#include "XSPI.h"


uint8_t read_flash_config(struct FlashConfig* config) {
    uint8_t config_buff[4];
    XSPI_Read(0, config_buff);
    XSPI_Read(0, config_buff);    // works by reading it twice
    uint32_t flash_config = unpack_uint32_le(config_buff);


    printf("Flash config: 0x%08x\n", flash_config);

    if (flash_config <= 0) {
        return 0;
    }

    config->controller_type = flash_config >> 17 & 3;
    config->block_type = flash_config >> 4 & 3;

    config->page_size = 0x200;
    config->pages_count = 0;
    config->blocks_count = 0;
    config->fs_blocks = 0;

    uint8_t ctrl = config->controller_type;
    uint8_t blk = config->block_type;
    if (ctrl == 0) {
        config->pages_count = 0x20;
        if (blk == 0) {
            printf("ctype 0 btype 0 is invalid!\n");
            return 0;
        } else if (blk == 1) {
            config->blocks_count = 0x400;
            config->fs_blocks = 0x3E0;
        } else if (blk == 2) {
            config->blocks_count = 0x800;
            config->fs_blocks = 0x7C0;
        } else if (blk == 3) {
            config->blocks_count = 0x1000;
            config->fs_blocks = 0xF80;
        }            
    } else if (ctrl == 1 && config->block_type == 0) {
        printf("ctype 1 btype 0 is invalid!\n");
        return 0;
    } else if ((ctrl == 1 || ctrl == 2) && 
                (blk == 0 || blk == 1)) {
        config->pages_count = 0x20;
        if (blk == 0 || (blk == 1 && ctrl == 1)) {
            config->blocks_count = 0x400;
            config->fs_blocks = 0x3E0;
        } else if (ctrl == 2 && blk == 1) {
            config->blocks_count = 0x1000;
            config->fs_blocks = 0xF80;
        }
    } else if ((ctrl == 1 || ctrl == 2) && 
                (blk == 2 || blk == 3)) {
        if (blk == 2) {
            config->pages_count = 0x100;
            config->blocks_count = 1 << ((flash_config >> 19 & 3) + (flash_config >> 21 & 15) + 23) >> 17;
            config->fs_blocks = 0x1E0;
        } else if (blk == 3) {
            config->pages_count = 0x200;
            config->blocks_count = 1 << ((flash_config >> 19 & 3) + (flash_config >> 21 & 15) + 23) >> 18;
            config->fs_blocks = 0xF0;
        }
    } else if (ctrl == 3 && config->block_type == 0) {
        printf("Corona 4GB unsupported\n");
        return 0;
    } else {
        printf("ctype %d btype %d is invalid!\n", ctrl, config->block_type);
        return 0;
    }

    return 1;
}

void print_flash_config(struct FlashConfig* config) {
    printf("Flash info:\n");
    uint8_t ctrl = config->controller_type;
    uint8_t blk = config->block_type;
    if (ctrl == 0 && blk == 1) {
        printf("\tXenon/Zephyr/Falcon/older Jasper 16MB\n");
    } else if (ctrl == 1 && blk == 1) {
        printf("\tnewer Jasper 16MB/Trinity\n");
    } else if (ctrl == 1 && blk == 2) {
        printf("\tJasper 256/512MB\n");
    } else if (ctrl == 2 && blk == 0) {
        printf("\tCorona 16MB\n");
    } else {
        printf("UNKNOWN CONSOLE");
    }

    printf("\tBlocks count: 0x%x\n", config->blocks_count);
    printf("\tPage size: 0x%x\n", config->page_size);
    uint32_t block_size = config->page_size * config->pages_count;
    printf("\tBlock size: 0x%x (%d KB)\n", block_size, (block_size / 1024)); 
    uint32_t nand_size = (block_size / 1024) * config->blocks_count;
    if (nand_size / 1024 < 1) {
        printf("\tNAND size: %d KB\n", nand_size); 
    } else {
        printf("\tNAND size: %d MB\n", nand_size / 1024); 
    }
    printf("\tFS blocks: 0x%04x\n", config->fs_blocks);
}

void read_nand(uint32_t start, uint32_t blocks, struct FlashConfig* config, uint8_t* buffer)
{
    piHiPri(30);

    printf("Reading flash blocks %04x-%04x...\n", start, blocks);
    uint32_t wordsLeft = 0;
    uint32_t nextPage = start << 5;
    uint32_t rawPageSize = config->page_size + CRC_DATA_PER_PAGE;
    uint32_t rawBlockSize = rawPageSize * config->pages_count;
    
    uint32_t len = (blocks - start) * (rawBlockSize / BYTES_PER_COMMAND);
    while (len) {
            uint8_t toRead;
       
            if (!wordsLeft) {
                XNAND_StartRead(nextPage);
                nextPage++;
                wordsLeft = rawPageSize / BYTES_PER_COMMAND;
            }
 
            toRead = (len < wordsLeft) ? len : wordsLeft;
            XNAND_ReadFillBuffer(buffer, toRead);
        
            buffer += (toRead*BYTES_PER_COMMAND);
            wordsLeft -= toRead;
            len -= toRead;

            if ((nextPage >> 5) % 4 == 0) {
                printf("0x%x\r", nextPage >> 5);
            }
    }
    
    printf("\nRead %04x/%04x blocks\n", (nextPage >> 5) - start, blocks - start);
    piHiPri(0);
}

uint32_t unpack_uint32_le(uint8_t* bytes) {
	return bytes[0] + (bytes[1] << 8) + (bytes[2] << 16) + (bytes[3] << 24);
}


