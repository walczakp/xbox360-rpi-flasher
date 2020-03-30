#include "XSPI.h"
#include "XNAND.h"
#include "unpack.h"
#include <stdio.h>
#include <stdlib.h>

uint8_t FlashConfig[4];

void cleanup(void) {
	printf("Leaving flashmode!\n");
	XSPI_LeaveFlashmode();
}

int main(void) {
	atexit(cleanup);

	printf("Initializing XBOX360 SPI...\n");
	XSPI_Init();

	printf("Entering flashmode...\n");
	XSPI_EnterFlashmode();

	printf("Reading flash config...\n");
	XSPI_Read(0, FlashConfig);
	XSPI_Read(0, FlashConfig);	// works by reading it twice
	
	uint32_t flash_config = unpack_uint32_le(FlashConfig);
	printf("Flash config: 0x%08x\n", flash_config);

	if (flash_config <= 0) {
		printf("Your flash config is incorrect, check your wiring!\n");
		return 1;
	}

	return 0;
}
