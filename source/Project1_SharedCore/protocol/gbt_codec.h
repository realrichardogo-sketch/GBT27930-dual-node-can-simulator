/*
 * protocol/gbt_codec.h - byte-aligned message codecs (M5, B1-11..B1-14).
 *
 * Messages: CHM, BHM, CRM, BRM payload, CML, BRO, CRO, BCL, BSD.
 * Excluded: BCP, CSD (M6, gated by K-5); CCS, BSM, BST, CST, BEM, CEM, BCS
 * (M7, gated by K-3); all transport protocol (M8, gated by K-4). BRM is
 * encoded/decoded as a 49-byte payload only; its transport is blocked by M8.
 *
 * Rules (GB/T 27930-2023):
 *   §5.3  low byte first
 *   §5.6  physical value = resolution x raw + offset
 *   §8.7  unsent optional parameters are filled with all 1s
 *   §8.8  messages are sent with the specified length (DLC = length, no
 *         padding to 8); undefined bits are filled with 1. All messages in
 *         this module are fully allocated, so no undefined bits occur.
 *
 * Structures hold RAW codes. Decoding only reports a status; it never changes
 * protocol state, issues output commands or latches faults (that is M9).
 * Encoding packs the given raw values without validating them, so fault
 * injection can build invalid frames on purpose.
 */
#ifndef GBT_CODEC_H
#define GBT_CODEC_H

#include <stddef.h>
#include <stdint.h>
#include "common/gbt_types.h"

typedef enum {
    GBT_CODEC_OK = 0,
    GBT_CODEC_E_PARAM,    /* NULL pointer, bad buffer size, value not representable */
    GBT_CODEC_E_FORMAT,   /* frame is not a 29-bit extended data frame */
    GBT_CODEC_E_ID,       /* identifier does not identify this message */
    GBT_CODEC_E_LENGTH,   /* DLC / payload length differs from the specified length */
    GBT_CODEC_E_VALUE,    /* BCL SPN3074 not 0x01 or 0x02 (B1-12) */
    GBT_CODEC_E_RANGE     /* BCL SPN3073 outside -400 A ~ 0 A (B1-13) */
} gbt_codec_status_t;

/* Specified lengths (Tables 3-7) */
#define GBT_LEN_CHM 3u
#define GBT_LEN_BHM 2u
#define GBT_LEN_CRM 8u
#define GBT_LEN_BRM 49u
#define GBT_LEN_CML 8u
#define GBT_LEN_BRO 1u
#define GBT_LEN_CRO 1u
#define GBT_LEN_BCL 5u
#define GBT_LEN_BSD 7u

/* Charging current: 0.1 A/bit, -400 A offset (§5.6; SPN2826, 2827, 3073 ...) */
#define GBT_CURRENT_RAW_AT_0A 4000u   /* DERIVED: (0 A + 400 A) / 0.1 A */

/* CHM SPN2600, V1.1: byte1 = 01H, byte2 = 01H, byte3 = 00H (Table 8; OQ-4) */
extern const uint8_t gbt_chm_version_v1_1[3];
/* BRM SPN2565, V1.10: byte1 = 0x01, byte2 = 0x01, byte3 = 0x00 (Table 11; OQ-4) */
extern const uint8_t gbt_brm_version_v1_10[3];

typedef struct { uint8_t version[3]; } gbt_chm_t;               /* SPN2600, bytes 1-3 */

typedef struct { uint16_t insulation_voltage_raw; } gbt_bhm_t;  /* SPN2601, 0.1 V/bit */

typedef struct {
    uint8_t  result;          /* SPN2560: 0x00 not identified, 0xAA identified */
    uint32_t charger_number;  /* SPN2561, bytes 2-5, 1/bit, offset 0 */
    uint8_t  region_present;  /* SPN2562 optional; 0 -> bytes 6-8 sent as 0xFF */
    uint8_t  region[3];       /* SPN2562 ASCII */
} gbt_crm_t;

typedef struct {
    uint8_t  version[3];          /* SPN2565, bytes 1-3 */
    uint8_t  battery_type;        /* SPN2566, byte 4 */
    uint16_t rated_capacity_raw;  /* SPN2567, bytes 5-6, 0.1 Ah/bit */
    uint16_t rated_voltage_raw;   /* SPN2568, bytes 7-8, 0.1 V/bit */
    /* Optional fields (Table 11 "O"); present = 0 -> all bytes 0xFF (§8.7) */
    uint8_t  manufacturer_present;  uint8_t manufacturer[4];  /* SPN2569, bytes 9-12  */
    uint8_t  pack_serial_present;   uint8_t pack_serial[4];   /* SPN2570, bytes 13-16 */
    uint8_t  prod_date_present;                               /* SPN2571, bytes 17-19 */
    uint8_t  prod_year_raw;       /* 1 year/bit, offset 1985 */
    uint8_t  prod_month;          /* 1-12 */
    uint8_t  prod_day;            /* 1-31 */
    uint8_t  charge_count_present;  uint32_t charge_count;    /* SPN2572, bytes 20-22, 24 bit */
    uint8_t  ownership_present;     uint8_t ownership;        /* SPN2573, byte 23 */
    uint8_t  reserved_present;      uint8_t reserved;         /* SPN2574, byte 24 */
    uint8_t  vin_present;           uint8_t vin[17];          /* SPN2575, bytes 25-41 */
    uint8_t  sw_version_present;    uint8_t sw_version[8];    /* SPN2576, bytes 42-49 */
} gbt_brm_t;

typedef struct {
    uint16_t max_voltage_raw;   /* SPN2824, 0.1 V/bit */
    uint16_t min_voltage_raw;   /* SPN2825, 0.1 V/bit */
    uint16_t max_current_raw;   /* SPN2826, 0.1 A/bit, -400 A */
    uint16_t min_current_raw;   /* SPN2827, 0.1 A/bit, -400 A */
} gbt_cml_t;

typedef struct { uint8_t ready; } gbt_bro_t;   /* SPN2829: 0x00, 0xAA, 0xFF */
typedef struct { uint8_t ready; } gbt_cro_t;   /* SPN2830: 0x00, 0xAA, 0xFF */

#define GBT_BCL_MODE_CV 0x01u   /* SPN3074 constant voltage */
#define GBT_BCL_MODE_CC 0x02u   /* SPN3074 constant current */
#define GBT_BCL_INVALID_CURRENT 0x01u   /* invalid_mask bit: SPN3073 out of range */
#define GBT_BCL_INVALID_MODE    0x02u   /* invalid_mask bit: SPN3074 undefined */

typedef struct {
    uint16_t voltage_demand_raw;  /* SPN3072, bytes 1-2, 0.1 V/bit */
    uint16_t current_demand_raw;  /* SPN3073, bytes 3-4, 0.1 A/bit, -400 A */
    uint8_t  mode;                /* SPN3074, byte 5 */
    uint8_t  invalid_mask;        /* set by decode only; GBT_BCL_INVALID_* */
} gbt_bcl_t;

typedef struct {
    uint8_t  soc;                /* SPN3601, 1 %/bit */
    uint16_t min_cell_v_raw;     /* SPN3602, 0.01 V/bit */
    uint16_t max_cell_v_raw;     /* SPN3603, 0.01 V/bit */
    uint8_t  min_temp_raw;       /* SPN3604, 1 degC/bit, -50 degC */
    uint8_t  max_temp_raw;       /* SPN3605, 1 degC/bit, -50 degC */
} gbt_bsd_t;

/* Encoders: build a complete extended frame with the derived identifier. */
gbt_codec_status_t gbt_encode_chm(const gbt_chm_t *m, gbt_can_frame_t *f);
gbt_codec_status_t gbt_encode_bhm(const gbt_bhm_t *m, gbt_can_frame_t *f);
gbt_codec_status_t gbt_encode_crm(const gbt_crm_t *m, gbt_can_frame_t *f);
gbt_codec_status_t gbt_encode_cml(const gbt_cml_t *m, gbt_can_frame_t *f);
gbt_codec_status_t gbt_encode_bro(const gbt_bro_t *m, gbt_can_frame_t *f);
gbt_codec_status_t gbt_encode_cro(const gbt_cro_t *m, gbt_can_frame_t *f);
gbt_codec_status_t gbt_encode_bcl(const gbt_bcl_t *m, gbt_can_frame_t *f);
gbt_codec_status_t gbt_encode_bsd(const gbt_bsd_t *m, gbt_can_frame_t *f);
/* BRM payload only (49 bytes); transport is M8 (blocked). */
gbt_codec_status_t gbt_encode_brm_payload(const gbt_brm_t *m, uint8_t *buf, size_t buf_len);

/* Decoders: check format, identifier and DLC, then decode raw fields. */
gbt_codec_status_t gbt_decode_chm(const gbt_can_frame_t *f, gbt_chm_t *m);
gbt_codec_status_t gbt_decode_bhm(const gbt_can_frame_t *f, gbt_bhm_t *m);
gbt_codec_status_t gbt_decode_crm(const gbt_can_frame_t *f, gbt_crm_t *m);
gbt_codec_status_t gbt_decode_cml(const gbt_can_frame_t *f, gbt_cml_t *m);
gbt_codec_status_t gbt_decode_bro(const gbt_can_frame_t *f, gbt_bro_t *m);
gbt_codec_status_t gbt_decode_cro(const gbt_can_frame_t *f, gbt_cro_t *m);
/* Returns E_RANGE if SPN3073 is out of range, else E_VALUE if SPN3074 is
 * undefined; all fields are decoded in either case and invalid_mask lists
 * every finding. */
gbt_codec_status_t gbt_decode_bcl(const gbt_can_frame_t *f, gbt_bcl_t *m);
gbt_codec_status_t gbt_decode_bsd(const gbt_can_frame_t *f, gbt_bsd_t *m);
gbt_codec_status_t gbt_decode_brm_payload(const uint8_t *buf, size_t len, gbt_brm_t *m);

/* Charging-current range check (§5.4; endpoints per B1-14 PROVISIONAL). */
uint8_t gbt_current_raw_is_valid(uint16_t raw);
/* Exact conversions (§5.6). Current magnitude in mA per D-02/D-06 (single
 * conversion); returns E_RANGE and leaves *ma untouched if raw is invalid. */
gbt_codec_status_t gbt_current_raw_to_magnitude_ma(uint16_t raw, uint32_t *ma);
uint32_t gbt_voltage_0p1v_raw_to_mv(uint16_t raw);
uint32_t gbt_voltage_0p01v_raw_to_mv(uint16_t raw);
int16_t  gbt_temp_raw_to_degc(uint8_t raw);

#endif /* GBT_CODEC_H */
