/**
 * @file utils.c
 * @brief Utility implementation
 */

#include "utils.h"

// Lookup table for nibble reversal
static const uint8_t lookup[16] = {0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
                                   0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF};

uint8_t reverse_byte(uint8_t n) {
  // Reverse upper and lower nibbles and swap them
  return (lookup[n & 0x0F] << 4) | lookup[n >> 4];
}

void reverse_buffer(uint8_t *buf, size_t len) {
  for (size_t i = 0; i < len; i++) {
    buf[i] = reverse_byte(buf[i]);
  }
}

void reverse_buffer_copy(const uint8_t *src, uint8_t *dst, size_t len) {
  for (size_t i = 0; i < len; i++) {
    dst[i] = reverse_byte(src[i]);
  }
}

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Simple file comparison
bool compare_files(const char *f1, const char *f2) {
  FILE *fp1 = fopen(f1, "rb");
  FILE *fp2 = fopen(f2, "rb");

  if (!fp1 || !fp2) {
    if (fp1)
      fclose(fp1);
    if (fp2)
      fclose(fp2);
    return false;
  }

  bool match = true;
  int ch1, ch2;
  do {
    ch1 = fgetc(fp1);
    ch2 = fgetc(fp2);
    if (ch1 != ch2) {
      match = false;
      break;
    }
  } while (ch1 != EOF && ch2 != EOF);

  fclose(fp1);
  fclose(fp2);
  return match;
}
