/**
 * @file utils.h
 * @brief Utility functions for Pi4Flasher (Bit reversal, etc)
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Reverse bits in a byte (0x80 -> 0x01)
 * Used because Xbox 360 SPI is LSB-first, but Pi hardware SPI is MSB-first.
 */
uint8_t reverse_byte(uint8_t b);

/**
 * @brief Reverse bits in a buffer in-place
 */
void reverse_buffer(uint8_t *buf, size_t len);

#include <stdbool.h>

/**
 * @brief Reverse bits from src to dst
 */
void reverse_buffer_copy(const uint8_t *src, uint8_t *dst, size_t len);

/**
 * @brief Compare two files for equality
 */
bool compare_files(const char *f1, const char *f2);

#endif // UTILS_H
