/**
 * @file main.c
 * @brief Main entry point for Pi4Flasher (Interactive v2.0)
 */

#include "gpio.h"
#include "protocol.h"
#include "utils.h"
#include "xnand.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

// 512 data + 16 spare
#define SECTOR_SIZE 528

void print_usage(const char *prog_name) {
  printf("Usage: %s [command] [args...]\n", prog_name);
  printf("Commands:\n");
  printf("  info              Show NAND flash configuration\n");
  printf("  read <file>       Read NAND to file (full dump with spares)\n");
  printf("  write <file>      Write file to NAND (full image with spares)\n");
  printf(
      "  server [device]   Start JRunner server mode (default: /dev/ttyGS0)\n");
}

/* --- Refactored Helpers for Interactive Mode --- */

void wait_key(void) {
  printf("\nPress ENTER to continue...");
  while (getchar() != '\n')
    ;
  getchar(); // Wait for actual enter if buffer was clean?
             // Simplified:
             // getchar();
}

int perform_read(const char *filename) {
  if (XNAND_Init() < 0) {
    fprintf(stderr, "[-] Failed to initialize NAND controller\n");
    return 1;
  }

  XNAND_Config conf = XNAND_GetConfig();
  XNAND_PrintConfig(conf);

  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    perror("[-] Failed to open output file");
    return 1;
  }

  uint32_t total_sectors = (conf.size_mb * 1024 * 1024) / 512;
  uint8_t buffer[512];
  uint8_t spare[16];

  printf("[*] Reading %d sectors to %s...\n", total_sectors, filename);

  for (uint32_t i = 0; i < total_sectors; i++) {
    if (XNAND_ReadSector(i, buffer, spare) < 0) {
      fprintf(stderr, "\n[-] Read error at sector 0x%X\n", i);
      fclose(fp);
      return 1;
    }

    fwrite(buffer, 1, 512, fp);
    fwrite(spare, 1, 16, fp);

    if (i % 256 == 0) {
      printf("\rSector 0x%X / 0x%X", i, total_sectors);
      fflush(stdout);
    }
  }

  printf("\n[+] Read complete!\n");
  fclose(fp);
  return 0;
}

int perform_write(const char *filename) {
  if (XNAND_Init() < 0) {
    fprintf(stderr, "[-] Failed to initialize NAND controller\n");
    return 1;
  }

  XNAND_Config conf = XNAND_GetConfig();
  XNAND_PrintConfig(conf);

  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    perror("[-] Failed to open input file");
    return 1;
  }

  fseek(fp, 0, SEEK_END);
  long filesize = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  uint32_t total_sectors = filesize / SECTOR_SIZE;
  if (filesize % SECTOR_SIZE != 0) {
    printf("[!] Warning: File size is not aligned to 528 bytes. Using %d "
           "sectors.\n",
           total_sectors);
  }

  printf("=== NAND WRITE ===\n");
  printf("Input file: %s\n", filename);
  printf("File size: %ld bytes (%.2f MB)\n", filesize,
         filesize / (1024.0 * 1024.0));
  printf("Sectors: %d (0x%X)\n", total_sectors, total_sectors);
  printf("Writing sectors 0x0 to 0x%X...\n", total_sectors - 1);

  uint8_t buffer[512];
  uint8_t spare[16];

  for (uint32_t i = 0; i < total_sectors; i++) {
    if (fread(buffer, 1, 512, fp) != 512 || fread(spare, 1, 16, fp) != 16) {
      fprintf(stderr, "\n[-] Unexpected end of file at sector %d\n", i);
      break;
    }

    if (XNAND_WriteSector(i, buffer, spare) < 0) {
      fprintf(stderr, "\n[-] Write error at sector 0x%X\n", i);
      fclose(fp);
      return 1;
    }

    if (i % 256 == 0) {
      printf("\rSector 0x%X / 0x%X", i, total_sectors - 1);
      fflush(stdout);
    }
  }

  printf("\nWrite complete: %d sectors\n", total_sectors);
  printf("SUCCESS: File written to NAND\n");
  fclose(fp);
  return 0;
}

/* --- Menu System --- */

void show_menu(void) {
  printf("\n");
  printf("+--------------------------------------------+\n");
  printf("|            READ OPERATIONS                 |\n");
  printf("|  [1] Read full NAND to file                |\n");
  printf("|  [2] Read NAND 3x (verify dumps)           |\n");
  printf("+--------------------------------------------+\n");
  printf("|           WRITE OPERATIONS                 |\n");
  printf("|  [3] Write glitch.ecc (XeLL boot)          |\n");
  printf("|  [4] Write updflash.bin (full image)       |\n");
  printf("|  [5] Write custom file                     |\n");
  printf("+--------------------------------------------+\n");
  printf("|  [6] Refresh console info                  |\n");
  printf("|  [0] Exit                                  |\n");
  printf("+--------------------------------------------+\n");
  printf("\nChoice: ");
}

void interactive_mode(void) {
  int choice;
  char buffer[256];

  // Auto-detect console on startup
  printf("+--------------------------------------------+\n");
  printf("|             CONNECTED CONSOLE              |\n");
  printf("+--------------------------------------------+\n");

  if (XNAND_Init() < 0) {
    printf("|  Status: NOT DETECTED                     |\n");
    printf("|  Check wiring and power                   |\n");
  } else {
    XNAND_Config conf = XNAND_GetConfig();
    const char *block_type =
        (conf.erase_block_size >= 0x20000) ? "Big Block" : "Small Block";
    printf("|  Status: CONNECTED                        |\n");
    printf("|  Flash Config: 0x%08X                  |\n", conf.flash_config);
    printf("|  Flash Size:   %-3d MB                     |\n", conf.size_mb);
    printf("|  Block Type:   %-26s |\n", block_type);
  }

  printf("+--------------------------------------------+\n");

  while (1) {

    show_menu();
    if (scanf("%d", &choice) != 1) {
      while (getchar() != '\n')
        ; // flush invalid input
      continue;
    }
    while (getchar() != '\n')
      ; // consume newline

    switch (choice) {
    case 1:
      printf("Enter output filename (default: nanddump.bin): ");
      if (fgets(buffer, sizeof(buffer), stdin) && buffer[0] != '\n') {
        buffer[strcspn(buffer, "\n")] = 0;
      } else {
        strcpy(buffer, "nanddump.bin");
      }
      perform_read(buffer);
      wait_key();
      break;

    case 2:
      printf("[*] Starting Read 1...\n");
      perform_read("nanddump1.bin");
      printf("[*] Starting Read 2...\n");
      perform_read("nanddump2.bin");
      printf("[*] Starting Read 3...\n");
      perform_read("nanddump3.bin");

      printf("[*] Verifying dumps...\n");
      if (compare_files("nanddump1.bin", "nanddump2.bin") &&
          compare_files("nanddump2.bin", "nanddump3.bin")) {
        printf("[+] VERIFICATION SUCCESS: All 3 dumps match!\n");
      } else {
        printf("[-] VERIFICATION FAILED: Dumps do not match!\n");
      }
      wait_key();
      break;

    case 3:
      perform_write("glitch.ecc");
      wait_key();
      break;

    case 4:
      perform_write("updflash.bin");
      wait_key();
      break;

    case 5:
      printf("Enter input filename: ");
      if (fgets(buffer, sizeof(buffer), stdin)) {
        buffer[strcspn(buffer, "\n")] = 0;
        if (strlen(buffer) > 0)
          perform_write(buffer);
      }
      wait_key();
      break;

    case 6:
      if (XNAND_Init() == 0) {
        XNAND_PrintConfig(XNAND_GetConfig());
      }
      wait_key();
      break;

    case 0:
      printf("Exiting...\n");
      return;

    default:
      printf("Invalid choice\n");
    }
  }
}

/* --- Legacy/CLI Wrappers --- */

int cmd_info(void) {
  if (XNAND_Init() < 0)
    return 1;
  XNAND_PrintConfig(XNAND_GetConfig());
  return 0;
}

int cmd_read(const char *filename) { return perform_read(filename); }

int cmd_write(const char *filename) { return perform_write(filename); }

int cmd_server(const char *device) {
  printf("[*] Starting Server on %s...\n", device);
  int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd < 0) {
    perror("[-] Failed to open device");
    if (strcmp(device, "-") == 0) {
      printf("[*] Using stdin/stdout\n");
      fd = 0;
      return 1;
    }
    return 1;
  }

  struct termios tty;
  if (tcgetattr(fd, &tty) == 0) {
    cfmakeraw(&tty);
    tcsetattr(fd, TCSANOW, &tty);
  }

  Protocol_Loop(fd);
  close(fd);
  return 0;
}

/* --- Main --- */

void print_banner(void) {
  printf("\n");
  printf("  ██████╗ ██╗███████╗██╗      █████╗ ███████╗██╗  ██╗███████╗██████╗ "
         "\n");
  printf("  ██╔══██╗██║██╔════╝██║     ██╔══██╗██╔════╝██║  "
         "██║██╔════╝██╔══██╗\n");
  printf("  ██████╔╝██║█████╗  ██║     ███████║███████╗███████║█████╗  "
         "██████╔╝\n");
  printf("  ██╔═══╝ ██║██╔══╝  ██║     ██╔══██║╚════██║██╔══██║██╔══╝  "
         "██╔══██╗\n");
  printf("  ██║     ██║██║     ███████╗██║  ██║███████║██║  ██║███████╗██║  "
         "██║\n");
  printf("  ╚═╝     ╚═╝╚═╝     ╚══════╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚══════╝╚═╝  "
         "╚═╝\n");
  printf("\n");
  printf("     //       Xbox 360 NAND Flasher for Raspberry Pi       //\n");
  printf("\n");
}

void select_pi_model(void) {
  int choice;
  printf("+--------------------------------------------+\n");
  printf("|         SELECT YOUR RASPBERRY PI           |\n");
  printf("+--------------------------------------------+\n");
  printf("|  [1] Raspberry Pi 4 (40-pin header)        |\n");
  printf("|  [2] Raspberry Pi 1 Model B (26-pin)       |\n");
  printf("+--------------------------------------------+\n");
  printf("\nChoice: ");

  if (scanf("%d", &choice) != 1) {
    choice = 1; // Default to Pi4
  }
  while (getchar() != '\n')
    ; // consume newline

  if (choice == 2) {
    GPIO_SetPiModel(PI_MODEL_1B);
  } else {
    GPIO_SetPiModel(PI_MODEL_4);
  }
  printf("\n");
}

int main(int argc, char *argv[]) {
  print_banner();

  // Select Pi model BEFORE GPIO init
  select_pi_model();

  if (GPIO_Init() < 0) {
    return 1;
  }

  // Default to interactive mode if no args
  if (argc < 2) {
    interactive_mode();
    GPIO_Shutdown();
    return 0;
  }

  int ret = 0;
  const char *cmd = argv[1];

  if (strcmp(cmd, "info") == 0) {
    ret = cmd_info();
  } else if (strcmp(cmd, "read") == 0) {
    if (argc < 3) {
      printf("Usage: %s read <filename>\n", argv[0]);
      ret = 1;
    } else {
      ret = cmd_read(argv[2]);
    }
  } else if (strcmp(cmd, "write") == 0) {
    if (argc < 3) {
      printf("Usage: %s write <filename>\n", argv[0]);
      ret = 1;
    } else {
      ret = cmd_write(argv[2]);
    }
  } else if (strcmp(cmd, "server") == 0) {
    const char *dev = "/dev/ttyGS0"; // Default USB Gadget Serial
    if (argc >= 3)
      dev = argv[2];
    ret = cmd_server(dev);
  } else {
    printf("Unknown command: %s\n", cmd);
    print_usage(argv[0]);
    ret = 1;
  }

  GPIO_Shutdown();
  return ret;
}
