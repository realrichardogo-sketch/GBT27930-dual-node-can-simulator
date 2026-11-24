/* protocol/gbt_bits.c - see gbt_bits.h (PROVISIONAL K-3 Candidate A). */
#include "protocol/gbt_bits.h"

#include <stddef.h>

static uint32_t first_bit(uint8_t start_byte, uint8_t start_bit)
{
    /* Candidate A: absolute bit index, bit 1 of byte 1 = index 0 = LSB of byte 1 */
    return ((uint32_t)start_byte - 1U) * 8U + ((uint32_t)start_bit - 1U);
}

static int valid(const void *buf, uint8_t start_byte, uint8_t start_bit, uint8_t width)
{
    return buf != NULL && start_byte >= 1U && start_bit >= 1U && start_bit <= 8U
        && width >= 1U && width <= 16U;
}

uint16_t gbt_bits_get(const uint8_t *buf, uint8_t start_byte, uint8_t start_bit, uint8_t width)
{
    uint32_t base;
    uint32_t i;
    uint32_t v = 0U;

    if (!valid(buf, start_byte, start_bit, width)) {
        return 0U;
    }
    base = first_bit(start_byte, start_bit);
    for (i = 0U; i < width; i++) {
        uint32_t bit = base + i;
        uint32_t b = ((uint32_t)buf[bit / 8U] >> (bit % 8U)) & 1U;
        v |= b << i;
    }
    return (uint16_t)v;
}

void gbt_bits_set(uint8_t *buf, uint8_t start_byte, uint8_t start_bit, uint8_t width, uint16_t value)
{
    uint32_t base;
    uint32_t i;

    if (!valid(buf, start_byte, start_bit, width)) {
        return;
    }
    base = first_bit(start_byte, start_bit);
    for (i = 0U; i < width; i++) {
        uint32_t bit = base + i;
        uint8_t mask = (uint8_t)(1U << (bit % 8U));
        if ((((uint32_t)value >> i) & 1U) != 0U) {
            buf[bit / 8U] = (uint8_t)(buf[bit / 8U] | mask);
        } else {
            buf[bit / 8U] = (uint8_t)(buf[bit / 8U] & (uint8_t)~mask);
        }
    }
}
