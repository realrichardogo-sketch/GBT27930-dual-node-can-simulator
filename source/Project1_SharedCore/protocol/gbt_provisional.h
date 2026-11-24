/*
 * protocol/gbt_provisional.h - isolated PROVISIONAL project interpretations.
 *
 * Every value here is a project decision, NOT a GB/T 27930-2023 normative
 * value. Changing one requires an Owner decision.
 */
#ifndef GBT_PROVISIONAL_H
#define GBT_PROVISIONAL_H

/*
 * B1-14 / U-R2 (PROVISIONAL): GB/T 27930-2023 §5.4 states the valid charging
 * current range as "-400 A ~ 0 A" without stating whether the endpoints are
 * included. Project interpretation: both endpoints are valid.
 *   1 = endpoint valid, 0 = endpoint invalid.
 */
#define GBT_PROV_UR2_MIN_ENDPOINT_VALID 1u   /* -400.0 A, raw 0x0000 */
#define GBT_PROV_UR2_MAX_ENDPOINT_VALID 1u   /*    0.0 A, raw 0x0FA0 */


/*
 * P-K3-01 (PROVISIONAL, K-3.i Candidate A): data-field bit numbering.
 * Implemented in protocol/gbt_bits.c only; this constant documents the
 * selected candidate and is asserted by tests/unit/test_bits.c.
 *   1 = bit 1 is the LSB of its byte, numbering continues low byte first.
 */
#define GBT_PROV_K3_CANDIDATE_A 1u

/*
 * P-K5-02 (PROVISIONAL, K-5 OQ-2): CSD SPN3613 charger-number offset.
 * Table 27 prints offset 1 with range 0..0xFFFFFFFF; CRM SPN2561 (same
 * quantity) uses offset 0. Project interpretation: offset 0.
 */
#define GBT_PROV_K5_SPN3613_OFFSET 0u

/*
 * P-K5-01 (PROVISIONAL, K-5 OQ-1): BCP SPN2817 is a current in A,
 * 0.1 A/bit, -400 A offset (note 2), not V as printed in the Table 12 cell.
 * P-K5-03 (PROVISIONAL, K-5 OQ-3): BCP SPN2822 is a voltage in V,
 * 0.1 V/bit, 0 V offset (note 7), not degC as printed in the Table 12 cell.
 * Both are expressed by the scaling helpers used for these fields in
 * protocol/gbt_codec_ext.c; no separate constant is needed.
 */

#endif /* GBT_PROVISIONAL_H */
