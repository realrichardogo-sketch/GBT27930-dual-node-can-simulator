/* tests/unit/test_bits.c - central bit utility; PROVISIONAL K-3 Candidate A (P-K3-01). */
#include "protocol/gbt_bits.h"
#include "protocol/gbt_provisional.h"
#include "tests/support/test_assert.h"

#include <string.h>

int main(void)
{
    uint8_t b[4];

    CHECK_EQ_U(GBT_PROV_K3_CANDIDATE_A, 1U);   /* the convention the code implements */

    memset(b, 0, sizeof b);
    gbt_bits_set(b, 1U, 1U, 1U, 1U);           /* bit 1 = LSB */
    CHECK_EQ_U(b[0], 0x01U);
    gbt_bits_set(b, 1U, 8U, 1U, 1U);           /* bit 8 = MSB */
    CHECK_EQ_U(b[0], 0x81U);

    memset(b, 0xFF, sizeof b);
    gbt_bits_set(b, 1U, 1U, 2U, 1U);           /* code "01" at x.1: value 1 -> bit1=1, bit2=0 */
    CHECK_EQ_U(b[0], 0xFDU);
    gbt_bits_set(b, 1U, 5U, 2U, 1U);           /* code "01" at x.5 */
    CHECK_EQ_U(b[0], 0xDDU);                    /* 1101 1101: bit5=1, bit6=0 */
    CHECK_EQ_U(gbt_bits_get(b, 1U, 5U, 2U), 1U);
    CHECK_EQ_U(gbt_bits_get(b, 1U, 7U, 2U), 3U);

    /* numbering continues into the next byte, low byte first */
    memset(b, 0, sizeof b);
    gbt_bits_set(b, 1U, 1U, 12U, 0x0ABCU);
    gbt_bits_set(b, 2U, 5U, 4U, 0x0DU);        /* = bits 13-16 of the 16-bit field */
    CHECK_EQ_U(b[0], 0xBCU);
    CHECK_EQ_U(b[1], 0xDAU);
    CHECK_EQ_U(gbt_bits_get(b, 1U, 1U, 16U), 0xDABCU);
    gbt_bits_set(b, 1U, 7U, 4U, 0x0FU);        /* field crossing a byte boundary */
    CHECK_EQ_U(gbt_bits_get(b, 1U, 7U, 4U), 0x0FU);
    CHECK_EQ_U(b[0] & 0xC0U, 0xC0U);
    CHECK_EQ_U(b[1] & 0x03U, 0x03U);

    /* invalid arguments are ignored */
    memset(b, 0x5A, sizeof b);
    gbt_bits_set(b, 0U, 1U, 2U, 3U);
    gbt_bits_set(b, 1U, 9U, 2U, 3U);
    gbt_bits_set(b, 1U, 1U, 0U, 3U);
    gbt_bits_set(b, 1U, 1U, 17U, 3U);
    gbt_bits_set(NULL, 1U, 1U, 2U, 3U);
    CHECK_EQ_U(b[0], 0x5AU);
    CHECK_EQ_U(gbt_bits_get(b, 0U, 1U, 2U), 0U);
    CHECK_EQ_U(gbt_bits_get(NULL, 1U, 1U, 2U), 0U);

    return TEST_SUMMARY("test_bits");
}
