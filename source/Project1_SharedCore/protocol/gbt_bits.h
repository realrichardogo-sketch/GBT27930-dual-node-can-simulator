/*
 * protocol/gbt_bits.h - the ONE place that maps GB/T "byte.bit" positions to
 * physical bits. Every bit-field codec uses these functions; no other module
 * may compute bit positions.
 *
 * PROVISIONAL (K-3.i, Candidate A; register entry P-K3-01):
 *   - byte numbers start at 1 (GB/T tables "起始字节或位");
 *   - bit 1 is the least significant bit of its byte;
 *   - the lowest-numbered bit of a field holds the field value's LSB;
 *   - bit numbering continues into the next byte, low byte first (§5.3),
 *     so "bits 1-16" of a 2-byte SPN form one little-endian 16-bit value.
 * GB/T 27930-2023 does not define bit numbering (U-P2); normative resolution
 * is K-3.ii (OPEN). Changing the convention means changing gbt_bits.c only.
 */
#ifndef GBT_BITS_H
#define GBT_BITS_H

#include <stdint.h>

/* Read a field of 1..16 bits. start_byte >= 1, start_bit 1..8 (may continue
 * into following bytes). Returns 0 for an invalid width or position. */
uint16_t gbt_bits_get(const uint8_t *buf, uint8_t start_byte, uint8_t start_bit, uint8_t width);

/* Write a field of 1..16 bits; bits of value above width are ignored. */
void gbt_bits_set(uint8_t *buf, uint8_t start_byte, uint8_t start_bit, uint8_t width, uint16_t value);

#endif /* GBT_BITS_H */
