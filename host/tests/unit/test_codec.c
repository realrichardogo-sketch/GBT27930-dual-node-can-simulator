/*
 * tests/unit/test_codec.c - byte-aligned codecs (M5): golden vectors, byte
 * order, scaling, DLC, fill, invalid values. Field positions per
 * docs/sources/01.2_PROTOCOL_TABLES.md §3.
 */
#include "protocol/gbt_codec.h"
#include "tests/support/test_assert.h"

#include <string.h>

static int bytes_eq(const uint8_t *a, const uint8_t *b, size_t n)
{
    return memcmp(a, b, n) == 0;
}

static void test_chm_bhm(void)
{
    gbt_chm_t chm;
    gbt_chm_t chm2;
    gbt_bhm_t bhm = { 0x1234U };
    gbt_bhm_t bhm2 = { 0U };
    gbt_can_frame_t f;
    static const uint8_t chm_bytes[3] = { 0x01U, 0x01U, 0x00U };

    memcpy(chm.version, gbt_chm_version_v1_1, 3U);
    CHECK(gbt_encode_chm(&chm, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.id, 0x1826F456u);              /* Lean Plan acceptance example */
    CHECK_EQ_U(f.dlc, 3U);                      /* §8.8: specified length, no padding */
    CHECK_EQ_U(f.flags, GBT_CAN_FLAG_EXT);
    CHECK(bytes_eq(f.data, chm_bytes, 3U));
    CHECK(gbt_decode_chm(&f, &chm2) == GBT_CODEC_OK);
    CHECK(bytes_eq(chm2.version, chm_bytes, 3U));

    CHECK(gbt_encode_bhm(&bhm, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.id, 0x182756F4u);
    CHECK_EQ_U(f.dlc, 2U);
    CHECK_EQ_U(f.data[0], 0x34U);               /* §5.3 low byte first */
    CHECK_EQ_U(f.data[1], 0x12U);
    CHECK(gbt_decode_bhm(&f, &bhm2) == GBT_CODEC_OK);
    CHECK_EQ_U(bhm2.insulation_voltage_raw, 0x1234U);
    CHECK_EQ_U(gbt_voltage_0p1v_raw_to_mv(7500U), 750000U);   /* 750.0 V */
}

static void test_crm(void)
{
    gbt_crm_t m;
    gbt_crm_t d;
    gbt_can_frame_t f;
    static const uint8_t absent[8] = { 0xAAU, 0x78U, 0x56U, 0x34U, 0x12U, 0xFFU, 0xFFU, 0xFFU };
    static const uint8_t present[8] = { 0x00U, 0x01U, 0x00U, 0x00U, 0x00U, 0x42U, 0x4AU, 0x31U };

    memset(&m, 0, sizeof m);
    m.result = 0xAAU;
    m.charger_number = 0x12345678u;
    m.region_present = 0U;
    CHECK(gbt_encode_crm(&m, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.id, 0x1801F456u);
    CHECK_EQ_U(f.dlc, 8U);
    CHECK(bytes_eq(f.data, absent, 8U));        /* §8.7 optional unsent -> 0xFF */
    CHECK(gbt_decode_crm(&f, &d) == GBT_CODEC_OK);
    CHECK_EQ_U(d.result, 0xAAU);
    CHECK_EQ_U(d.charger_number, 0x12345678u);
    CHECK_EQ_U(d.region_present, 0U);

    m.result = 0x00U;
    m.charger_number = 1U;
    m.region_present = 1U;
    m.region[0] = 'B';
    m.region[1] = 'J';
    m.region[2] = '1';
    CHECK(gbt_encode_crm(&m, &f) == GBT_CODEC_OK);
    CHECK(bytes_eq(f.data, present, 8U));
    CHECK(gbt_decode_crm(&f, &d) == GBT_CODEC_OK);
    CHECK_EQ_U(d.region_present, 1U);
    CHECK_EQ_U(d.region[1], 'J');
}

static void test_brm_payload(void)
{
    gbt_brm_t m;
    gbt_brm_t d;
    uint8_t buf[GBT_LEN_BRM];
    unsigned i;
    unsigned ff = 0U;

    memset(&m, 0, sizeof m);
    memcpy(m.version, gbt_brm_version_v1_10, 3U);
    m.battery_type = 0x03U;            /* LiFePO4 */
    m.rated_capacity_raw = 1500U;      /* 150.0 Ah */
    m.rated_voltage_raw = 3500U;       /* 350.0 V */
    CHECK(gbt_encode_brm_payload(&m, buf, sizeof buf) == GBT_CODEC_OK);
    CHECK_EQ_U(buf[0], 0x01U);
    CHECK_EQ_U(buf[1], 0x01U);
    CHECK_EQ_U(buf[2], 0x00U);
    CHECK_EQ_U(buf[3], 0x03U);
    CHECK_EQ_U(buf[4], 0xDCU);         /* 1500 = 0x05DC */
    CHECK_EQ_U(buf[5], 0x05U);
    CHECK_EQ_U(buf[6], 0xACU);         /* 3500 = 0x0DAC */
    CHECK_EQ_U(buf[7], 0x0DU);
    for (i = 8U; i < GBT_LEN_BRM; i++) {
        ff += (buf[i] == 0xFFU) ? 1U : 0U;
    }
    CHECK_EQ_U(ff, 41U);               /* every optional byte filled (§8.7) */
    CHECK(gbt_decode_brm_payload(buf, sizeof buf, &d) == GBT_CODEC_OK);
    CHECK_EQ_U(d.rated_capacity_raw, 1500U);
    CHECK_EQ_U(d.vin_present, 0U);
    CHECK_EQ_U(d.charge_count_present, 0U);

    m.vin_present = 1U;
    memcpy(m.vin, "LSVAA4180E2123456", 17U);
    m.charge_count_present = 1U;
    m.charge_count = 0x00ABCDEFu;
    m.prod_date_present = 1U;
    m.prod_year_raw = 38U;             /* 1985 + 38 = 2023 */
    m.prod_month = 9U;
    m.prod_day = 7U;
    CHECK(gbt_encode_brm_payload(&m, buf, sizeof buf) == GBT_CODEC_OK);
    CHECK_EQ_U(buf[16], 38U);
    CHECK_EQ_U(buf[19], 0xEFU);        /* SPN2572 bytes 20-22, low byte first */
    CHECK_EQ_U(buf[21], 0xABU);
    CHECK_EQ_U(buf[24], 'L');          /* SPN2575 starts at byte 25 */
    CHECK_EQ_U(buf[40], '6');
    CHECK_EQ_U(buf[41], 0xFFU);        /* SPN2576 absent */
    CHECK(gbt_decode_brm_payload(buf, sizeof buf, &d) == GBT_CODEC_OK);
    CHECK_EQ_U(d.charge_count, 0x00ABCDEFu);
    CHECK_EQ_U(d.prod_month, 9U);
    CHECK(memcmp(d.vin, "LSVAA4180E2123456", 17U) == 0);

    CHECK(gbt_encode_brm_payload(&m, buf, 48U) == GBT_CODEC_E_PARAM);
    CHECK(gbt_decode_brm_payload(buf, 48U, &d) == GBT_CODEC_E_LENGTH);
    m.charge_count = 0x01000000u;
    CHECK(gbt_encode_brm_payload(&m, buf, sizeof buf) == GBT_CODEC_E_PARAM);
}

static void test_cml_bro_cro_bsd(void)
{
    gbt_cml_t cml = { 7500U, 2000U, 1500U, 3990U };   /* 750 V, 200 V, -250 A, -1 A */
    gbt_cml_t c2;
    gbt_bro_t bro = { 0xAAU };
    gbt_bro_t b2 = { 0U };
    gbt_cro_t cro = { 0x00U };
    gbt_cro_t r2 = { 0xFFU };
    gbt_bsd_t bsd = { 95U, 330U, 365U, 70U, 85U };
    gbt_bsd_t s2;
    gbt_can_frame_t f;
    static const uint8_t cml_bytes[8] = { 0x4CU, 0x1DU, 0xD0U, 0x07U, 0xDCU, 0x05U, 0x96U, 0x0FU };
    static const uint8_t bsd_bytes[7] = { 95U, 0x4AU, 0x01U, 0x6DU, 0x01U, 70U, 85U };

    CHECK(gbt_encode_cml(&cml, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.id, 0x1808F456u);
    CHECK_EQ_U(f.dlc, 8U);
    CHECK(bytes_eq(f.data, cml_bytes, 8U));
    CHECK(gbt_decode_cml(&f, &c2) == GBT_CODEC_OK);
    CHECK_EQ_U(c2.min_current_raw, 3990U);

    CHECK(gbt_encode_bro(&bro, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.id, 0x100956F4u);
    CHECK_EQ_U(f.dlc, 1U);
    CHECK(gbt_decode_bro(&f, &b2) == GBT_CODEC_OK);
    CHECK_EQ_U(b2.ready, 0xAAU);

    CHECK(gbt_encode_cro(&cro, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.id, 0x100AF456u);
    CHECK(gbt_decode_cro(&f, &r2) == GBT_CODEC_OK);
    CHECK_EQ_U(r2.ready, 0x00U);

    CHECK(gbt_encode_bsd(&bsd, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.id, 0x181C56F4u);
    CHECK_EQ_U(f.dlc, 7U);
    CHECK(bytes_eq(f.data, bsd_bytes, 7U));
    CHECK(gbt_decode_bsd(&f, &s2) == GBT_CODEC_OK);
    CHECK_EQ_U(s2.max_cell_v_raw, 365U);
    CHECK_EQ_U(gbt_voltage_0p01v_raw_to_mv(365U), 3650U);
    CHECK(gbt_temp_raw_to_degc(70U) == 20);
    CHECK(gbt_temp_raw_to_degc(0U) == -50);
}

static void test_bcl(void)
{
    gbt_bcl_t m = { 4000U, 3000U, GBT_BCL_MODE_CC, 0U };   /* 400.0 V, -100.0 A, CC */
    gbt_bcl_t d;
    gbt_can_frame_t f;
    uint32_t ma = 0U;
    static const uint8_t golden[5] = { 0xA0U, 0x0FU, 0xB8U, 0x0BU, 0x02U };

    CHECK(gbt_encode_bcl(&m, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.id, 0x181056F4u);
    CHECK_EQ_U(f.dlc, 5U);
    CHECK(bytes_eq(f.data, golden, 5U));           /* byte 3 low (B8), byte 4 high (0B) */
    CHECK(gbt_decode_bcl(&f, &d) == GBT_CODEC_OK);
    CHECK_EQ_U(d.current_demand_raw, 3000U);
    CHECK_EQ_U(d.invalid_mask, 0U);
    CHECK(gbt_current_raw_to_magnitude_ma(d.current_demand_raw, &ma) == GBT_CODEC_OK);
    CHECK_EQ_U(ma, 100000U);                        /* 100 A magnitude (D-02) */

    /* B1-13 boundaries; endpoints valid per B1-14 (PROVISIONAL) */
    CHECK_EQ_U(gbt_current_raw_is_valid(0x0000U), 1U);   /* PROVISIONAL: -400.0 A */
    CHECK_EQ_U(gbt_current_raw_is_valid(0x0001U), 1U);
    CHECK_EQ_U(gbt_current_raw_is_valid(0x0F9FU), 1U);
    CHECK_EQ_U(gbt_current_raw_is_valid(0x0FA0U), 1U);   /* PROVISIONAL: 0.0 A */
    CHECK_EQ_U(gbt_current_raw_is_valid(0x0FA1U), 0U);
    CHECK_EQ_U(gbt_current_raw_is_valid(0xFFFFU), 0U);   /* no U-R1 special code */
    CHECK(gbt_current_raw_to_magnitude_ma(0U, &ma) == GBT_CODEC_OK);
    CHECK_EQ_U(ma, 400000U);
    CHECK(gbt_current_raw_to_magnitude_ma(4000U, &ma) == GBT_CODEC_OK);
    CHECK_EQ_U(ma, 0U);
    ma = 7U;
    CHECK(gbt_current_raw_to_magnitude_ma(4001U, &ma) == GBT_CODEC_E_RANGE);
    CHECK_EQ_U(ma, 7U);

    m.current_demand_raw = 0x0FA1U;
    CHECK(gbt_encode_bcl(&m, &f) == GBT_CODEC_OK);  /* encoder does not validate */
    CHECK(gbt_decode_bcl(&f, &d) == GBT_CODEC_E_RANGE);
    CHECK_EQ_U(d.invalid_mask, GBT_BCL_INVALID_CURRENT);
    CHECK_EQ_U(d.current_demand_raw, 0x0FA1U);      /* still decoded */

    m.current_demand_raw = 3000U;
    m.mode = 0x03U;
    CHECK(gbt_encode_bcl(&m, &f) == GBT_CODEC_OK);
    CHECK(gbt_decode_bcl(&f, &d) == GBT_CODEC_E_VALUE);
    CHECK_EQ_U(d.invalid_mask, GBT_BCL_INVALID_MODE);

    m.current_demand_raw = 0xFFFFU;
    m.mode = 0x00U;
    CHECK(gbt_encode_bcl(&m, &f) == GBT_CODEC_OK);
    CHECK(gbt_decode_bcl(&f, &d) == GBT_CODEC_E_RANGE);
    CHECK_EQ_U(d.invalid_mask, GBT_BCL_INVALID_CURRENT | GBT_BCL_INVALID_MODE);
}

static void test_frame_checks(void)
{
    gbt_bcl_t m = { 4000U, 3000U, GBT_BCL_MODE_CV, 0U };
    gbt_bcl_t d;
    gbt_chm_t c;
    gbt_can_frame_t f;
    gbt_can_frame_t g;

    CHECK(gbt_encode_bcl(&m, &f) == GBT_CODEC_OK);
    g = f;
    g.dlc = 8U;
    CHECK(gbt_decode_bcl(&g, &d) == GBT_CODEC_E_LENGTH);   /* padded to 8: rejected */
    g.dlc = 4U;
    CHECK(gbt_decode_bcl(&g, &d) == GBT_CODEC_E_LENGTH);
    g = f;
    g.flags = 0U;
    CHECK(gbt_decode_bcl(&g, &d) == GBT_CODEC_E_FORMAT);   /* standard frame */
    g.flags = 0x03U;
    CHECK(gbt_decode_bcl(&g, &d) == GBT_CODEC_E_FORMAT);   /* unknown flag */
    CHECK(gbt_decode_chm(&f, &c) == GBT_CODEC_E_ID);       /* BCL frame to CHM decoder */
    g = f;
    g.id = 0x1810F456u;                                    /* BCL PF, wrong direction */
    CHECK(gbt_decode_bcl(&g, &d) == GBT_CODEC_E_ID);
    CHECK(gbt_decode_bcl(NULL, &d) == GBT_CODEC_E_PARAM);
    CHECK(gbt_decode_bcl(&f, NULL) == GBT_CODEC_E_PARAM);
    CHECK(gbt_encode_bcl(NULL, &f) == GBT_CODEC_E_PARAM);
    CHECK(gbt_encode_bcl(&m, NULL) == GBT_CODEC_E_PARAM);
}

int main(void)
{
    test_chm_bhm();
    test_crm();
    test_brm_payload();
    test_cml_bro_cro_bsd();
    test_bcl();
    test_frame_checks();
    return TEST_SUMMARY("test_codec");
}
