#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_
#include <inttypes.h>

#define CRC_DATA_PER_PAGE 0x10

struct FlashConfig {
    uint8_t controller_type;
    uint8_t block_type;

    uint16_t page_size;
    uint16_t pages_count;
    uint16_t blocks_count;
    uint16_t fs_blocks;
};

uint8_t read_flash_config(struct FlashConfig* config);
void print_flash_config(struct FlashConfig* config);
void read_nand(uint32_t start, uint32_t blocks, struct FlashConfig* config, uint8_t* buffer);
uint32_t unpack_uint32_le(uint8_t* bytes);

#endif 