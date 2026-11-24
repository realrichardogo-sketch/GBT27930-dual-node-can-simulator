/*
 * protocol/gbt_codec_ext.h - codecs that depend on PROVISIONAL decisions.
 *
 *   Bit-field messages (K-3.i Candidate A via protocol/gbt_bits.c):
 *     CCS, BSM, BST, CST, BEM, CEM, and BCS SPN3077.
 *   Provisional rows (K-5): BCP SPN2817 (OQ-1), SPN2822 (OQ-3); CSD SPN3613 (OQ-2).
 *   TP payloads (transport itself is protocol/gbt_tp): BCP 13 B, BCS 9 B.
 *
 * Every frame/payload starts from all 1s, so undefined bits and reserved
 * fields are 1 (§8.7, §8.8); defined fields are then written. Two-bit status
 * codes are carried as raw values 0..3; 0b11 in a defined field is reported
 * as GBT_CODEC_E_VALUE (GB/T defines only 00, 01, 10).
 * Register: docs/PROVISIONAL_PARAMETERS.md.
 */
#ifndef GBT_CODEC_EXT_H
#define GBT_CODEC_EXT_H

#include <stddef.h>
#include <stdint.h>
#include "common/gbt_types.h"
#include "protocol/gbt_codec.h"

#define GBT_LEN_CCS 7u
#define GBT_LEN_BSM 7u
#define GBT_LEN_BST 4u
#define GBT_LEN_CST 4u
#define GBT_LEN_BEM 4u
#define GBT_LEN_CEM 4u
#define GBT_LEN_CSD 8u
#define GBT_LEN_BCP 13u
#define GBT_LEN_BCS 9u

/* Two-bit status codes (Tables 19, 20, 24, 25, 28, 29) */
#define GBT_CODE_00 0u
#define GBT_CODE_01 1u
#define GBT_CODE_10 2u   /* "not credible" where defined */

/* CCS, Table 19 */
typedef struct {
    uint16_t voltage_raw;        /* SPN3081, 0.1 V/bit */
    uint16_t current_raw;        /* SPN3082, 0.1 A/bit, -400 A */
    uint16_t charge_time_min;    /* SPN3083, 1 min/bit, 0..600 */
    uint8_t  permit;             /* SPN3929 (7.1, 2 bit): 00 pause, 01 permit */
} gbt_ccs_t;

/* BSM, Table 20 */
#define GBT_BSM_STATUS_COUNT 7u   /* SPN3090..SPN3096 */
typedef struct {
    uint8_t max_cell_v_index_raw; /* SPN3085, 1/bit, offset 1 */
    uint8_t max_temp_raw;         /* SPN3086, 1 degC/bit, -50 */
    uint8_t max_temp_point_raw;   /* SPN3087, offset 1 */
    uint8_t min_temp_raw;         /* SPN3088, 1 degC/bit, -50 */
    uint8_t min_temp_point_raw;   /* SPN3089, offset 1 */
    uint8_t status[GBT_BSM_STATUS_COUNT]; /* [0]=SPN3090 ... [6]=SPN3096 */
} gbt_bsm_t;
#define GBT_BSM_IDX_CHARGE_PERMIT 6u   /* SPN3096: 00 prohibited, 01 permitted */

/* BST (Table 24) / CST (Table 25): same layout */
#define GBT_STOP_REASON_GROUPS 4u     /* SPN3511/3521: byte 1, bits 1-8 */
#define GBT_STOP_FAULT_GROUPS  8u     /* SPN3512/3522: bytes 2-3, bits 1-16 */
#define GBT_STOP_ERROR_GROUPS  3u     /* SPN3513/3523: byte 4, bits 1-6 */
typedef struct {
    uint8_t reason[GBT_STOP_REASON_GROUPS];
    uint8_t fault[GBT_STOP_FAULT_GROUPS];
    uint8_t error[GBT_STOP_ERROR_GROUPS];
} gbt_stop_t;

/* BEM (Table 28): SPN3901..3907; CEM (Table 29): SPN3921..3928 */
#define GBT_BEM_FIELDS 7u
#define GBT_CEM_FIELDS 8u
typedef struct { uint8_t code[GBT_BEM_FIELDS]; } gbt_bem_t;
typedef struct { uint8_t code[GBT_CEM_FIELDS]; } gbt_cem_t;
/* Indices by meaning (for the timeout handling) */
#define GBT_BEM_CRM00 0u   /* SPN3901 */
#define GBT_BEM_CRMAA 1u   /* SPN3902 */
#define GBT_BEM_CML   2u   /* SPN3903 */
#define GBT_BEM_CRO   3u   /* SPN3904 */
#define GBT_BEM_CCS   4u   /* SPN3905 */
#define GBT_BEM_CST   5u   /* SPN3906 */
#define GBT_BEM_CSD   6u   /* SPN3907 */
#define GBT_CEM_BRM   0u   /* SPN3921 */
#define GBT_CEM_BCP   1u   /* SPN3922 */
#define GBT_CEM_BRO   2u   /* SPN3923 */
#define GBT_CEM_BCS   3u   /* SPN3924 */
#define GBT_CEM_BCL   4u   /* SPN3925 */
#define GBT_CEM_BST   5u   /* SPN3926 */
#define GBT_CEM_BSD   6u   /* SPN3927 */
#define GBT_CEM_BSM   7u   /* SPN3928 */

/* CSD, Table 27 */
typedef struct {
    uint16_t charge_time_min;     /* SPN3611 */
    uint16_t energy_raw;          /* SPN3612, 0.1 kWh/bit */
    uint32_t charger_number_raw;  /* SPN3613; physical = raw + P-K5-02 offset */
} gbt_csd_t;

/* BCP payload, Table 12 (13 bytes, via TP) */
typedef struct {
    uint16_t max_cell_v_raw;      /* SPN2816, 0.01 V/bit */
    uint16_t max_current_raw;     /* SPN2817, 0.1 A/bit, -400 A (PROVISIONAL P-K5-01) */
    uint16_t energy_raw;          /* SPN2818, 0.1 kWh/bit */
    uint16_t max_voltage_raw;     /* SPN2819, 0.1 V/bit */
    uint8_t  max_temp_raw;        /* SPN2820, 1 degC/bit, -50 */
    uint16_t soc_raw;             /* SPN2821, 0.1 %/bit */
    uint16_t present_voltage_raw; /* SPN2822, 0.1 V/bit (PROVISIONAL P-K5-03) */
} gbt_bcp_t;

/* BCS payload, Table 18 (9 bytes, via TP) */
typedef struct {
    uint16_t voltage_raw;         /* SPN3075, 0.1 V/bit */
    uint16_t current_raw;         /* SPN3076, 0.1 A/bit, -400 A */
    uint16_t max_cell_v_raw;      /* SPN3077 bits 1-12, 0.01 V/bit (K-3 PROVISIONAL) */
    uint8_t  max_cell_group;      /* SPN3077 bits 13-16 */
    uint8_t  soc;                 /* SPN3078, 1 %/bit */
    uint16_t remaining_min;       /* SPN3079 */
} gbt_bcs_t;

gbt_codec_status_t gbt_encode_ccs(const gbt_ccs_t *m, gbt_can_frame_t *f);
gbt_codec_status_t gbt_decode_ccs(const gbt_can_frame_t *f, gbt_ccs_t *m);  /* E_RANGE current, E_VALUE code */
gbt_codec_status_t gbt_encode_bsm(const gbt_bsm_t *m, gbt_can_frame_t *f);
gbt_codec_status_t gbt_decode_bsm(const gbt_can_frame_t *f, gbt_bsm_t *m);
gbt_codec_status_t gbt_encode_bst(const gbt_stop_t *m, gbt_can_frame_t *f);
gbt_codec_status_t gbt_decode_bst(const gbt_can_frame_t *f, gbt_stop_t *m);
gbt_codec_status_t gbt_encode_cst(const gbt_stop_t *m, gbt_can_frame_t *f);
gbt_codec_status_t gbt_decode_cst(const gbt_can_frame_t *f, gbt_stop_t *m);
gbt_codec_status_t gbt_encode_bem(const gbt_bem_t *m, gbt_can_frame_t *f);
gbt_codec_status_t gbt_decode_bem(const gbt_can_frame_t *f, gbt_bem_t *m);
gbt_codec_status_t gbt_encode_cem(const gbt_cem_t *m, gbt_can_frame_t *f);
gbt_codec_status_t gbt_decode_cem(const gbt_can_frame_t *f, gbt_cem_t *m);
gbt_codec_status_t gbt_encode_csd(const gbt_csd_t *m, gbt_can_frame_t *f);
gbt_codec_status_t gbt_decode_csd(const gbt_can_frame_t *f, gbt_csd_t *m);
gbt_codec_status_t gbt_encode_bcp_payload(const gbt_bcp_t *m, uint8_t *buf, size_t buf_len);
gbt_codec_status_t gbt_decode_bcp_payload(const uint8_t *buf, size_t len, gbt_bcp_t *m); /* E_RANGE SPN2817 */
gbt_codec_status_t gbt_encode_bcs_payload(const gbt_bcs_t *m, uint8_t *buf, size_t buf_len);
gbt_codec_status_t gbt_decode_bcs_payload(const uint8_t *buf, size_t len, gbt_bcs_t *m); /* E_RANGE SPN3076 */

/* CSD SPN3613 physical value (P-K5-02). */
uint32_t gbt_csd_charger_number(uint32_t raw);

#endif /* GBT_CODEC_EXT_H */
