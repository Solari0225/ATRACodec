#include "put_words.h"

static uint32_t arithmetic_shift_right(uint32_t value, unsigned shift)
{
    if (shift == 0) return value;
    return (value >> shift) |
        ((value & UINT32_C(0x80000000)) ? (UINT32_MAX << (32 - shift)) : 0);
}

void sony_recovered_atrac3_put24(uint32_t word, uint32_t bit_position,
                               uint8_t *output)
{
    uint8_t *destination = output + (bit_position >> 3);
    const uint32_t shifted = arithmetic_shift_right(word, bit_position & 7);
    destination[2] = (uint8_t)shifted;
    destination[1] = (uint8_t)(shifted >> 8);
    destination[0] |= (uint8_t)(shifted >> 16);
}

void sony_recovered_atrac3_put32(uint32_t word, uint32_t bit_position,
                               uint8_t *output)
{
    uint8_t *destination = output + (bit_position >> 3);
    const uint32_t shifted = word >> (bit_position & 7);
    destination[3] = (uint8_t)shifted;
    destination[2] = (uint8_t)(shifted >> 8);
    destination[1] = (uint8_t)(shifted >> 16);
    destination[0] |= (uint8_t)(shifted >> 24);
}
