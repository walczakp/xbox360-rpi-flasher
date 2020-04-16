#include "unpack.h"

uint32_t unpack_uint32_le(uint8_t *bytes) {
  return bytes[0] + (bytes[1] << 8) + (bytes[2] << 16) + (bytes[3] << 24);
}
