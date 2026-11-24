/*
 * tests/unit/test_codec_ext.c - bit-field and provisional-row codecs.
 * ALL golden byte values depend on PROVISIONAL decisions:
 *   P-K3-01 (bit numbering, Candidate A), P-K5-01/02/03 (BCP, CSD rows).
 * If a decision changes, these vectors must be re-derived.
 */
#include "protocol/gbt_codec_ext.h"
#include "tests/support/test_assert.h"

#include <string.h>

static int eq(const uint8_t *a, const uint8_t *b, size_t n)
{
    return memcmp(a, b, n) == 0;
}

static void test_ccs(void)
{
    gbt_ccs_t m = { 4000U, 3000U, 30U, GBT_CODE_01 };
    gbt_ccs_t d;
    gbt_can_frame_t f;
    static const uint8_t g[7] = { 0xA0U, 0x0FU, 0xB8U, 0x0BU, 0x1EU, 0x00U, 0xFDU };

    CHECK(gbt_encode_ccs(&m, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.id, 0x1812F456u);
    CHECK_EQ_U(f.dlc, 7U);
    CHECK(eq(f.data, g, 7U));                  /* SPN3929=01, bits 3-8 filled with 1 */
    CHECK(gbt_decode_ccs(&f, &d) == GBT_CODEC_OK);
    CHECK_EQ_U(d.permit, GBT_CODE_01);
    CHECK_EQ_U(d.charge_time_min, 30U);

    m.permit = GBT_CODE_00;                    /* pause */
    CHECK(gbt_encode_ccs(&m, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.data[6], 0xFCU);
    m.permit = GBT_CODE_10;
    CHECK(gbt_encode_ccs(&m, &f) == GBT_CODEC_OK);
    CHECK(gbt_decode_ccs(&f, &d) == GBT_CODEC_E_VALUE);
    m.permit = GBT_CODE_01;
    m.current_raw = 4001U;
    CHECK(gbt_encode_ccs(&m, &f) == GBT_CODEC_OK);
    CHECK(gbt_decode_ccs(&f, &d) == GBT_CODEC_E_RANGE);
}

static void test_bsm(void)
{
    gbt_bsm_t m;
    gbt_bsm_t d;
    gbt_can_frame_t f;
    static const uint8_t g[7] = { 0x10U, 75U, 0x03U, 70U, 0x05U, 0x00U, 0xD0U };

    memset(&m, 0, sizeof m);
    m.max_cell_v_index_raw = 0x10U;
    m.max_temp_raw = 75U;
    m.max_temp_point_raw = 0x03U;
    m.min_temp_raw = 70U;
    m.min_temp_point_raw = 0x05U;
    m.status[GBT_BSM_IDX_CHARGE_PERMIT] = GBT_CODE_01;
    CHECK(gbt_encode_bsm(&m, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.id, 0x181356F4u);
    CHECK(eq(f.data, g, 7U));                  /* byte 7: 3094=00 3095=00 3096=01, bits 7-8 = 1 */
    CHECK(gbt_decode_bsm(&f, &d) == GBT_CODEC_OK);
    CHECK_EQ_U(d.status[6], GBT_CODE_01);

    m.status[0] = GBT_CODE_01;                 /* SPN3090 too high */
    m.status[3] = GBT_CODE_10;                 /* SPN3093 not credible */
    CHECK(gbt_encode_bsm(&m, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.data[5], 0x81U);
    CHECK(gbt_decode_bsm(&f, &d) == GBT_CODEC_OK);
    CHECK_EQ_U(d.status[3], GBT_CODE_10);

    m.status[GBT_BSM_IDX_CHARGE_PERMIT] = GBT_CODE_10;   /* undefined for SPN3096 */
    CHECK(gbt_encode_bsm(&m, &f) == GBT_CODEC_OK);
    CHECK(gbt_decode_bsm(&f, &d) == GBT_CODEC_E_VALUE);
}

static void test_bst_cst(void)
{
    gbt_stop_t m;
    gbt_stop_t d;
    gbt_can_frame_t f;
    static const uint8_t bst[4] = { 0x01U, 0x00U, 0x00U, 0xC0U };
    static const uint8_t cst[4] = { 0x10U, 0x00U, 0x04U, 0xC0U };

    memset(&m, 0, sizeof m);
    m.reason[0] = GBT_CODE_01;                 /* SPN3511 bits 1-2: required SOC reached */
    CHECK(gbt_encode_bst(&m, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.id, 0x101956F4u);
    CHECK_EQ_U(f.dlc, 4U);
    CHECK(eq(f.data, bst, 4U));
    CHECK(gbt_decode_bst(&f, &d) == GBT_CODEC_OK);
    CHECK_EQ_U(d.reason[0], GBT_CODE_01);

    memset(&m, 0, sizeof m);
    m.reason[2] = GBT_CODE_01;                 /* SPN3521 bits 5-6: fault stop (S-1) */
    m.fault[5] = GBT_CODE_01;                  /* SPN3522 bits 11-12: other fault */
    CHECK(gbt_encode_cst(&m, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.id, 0x101AF456u);
    CHECK(eq(f.data, cst, 4U));
    CHECK(gbt_decode_cst(&f, &d) == GBT_CODEC_OK);
    CHECK_EQ_U(d.fault[5], GBT_CODE_01);
    CHECK_EQ_U(d.reason[2], GBT_CODE_01);
    CHECK(gbt_decode_bst(&f, &d) == GBT_CODEC_E_ID);     /* CST frame to BST decoder */

    f.data[0] = 0x03U;                          /* code 11 in a defined group */
    CHECK(gbt_decode_cst(&f, &d) == GBT_CODEC_E_VALUE);
}

static void test_bem_cem(void)
{
    gbt_bem_t b;
    gbt_cem_t c;
    gbt_bem_t bd;
    gbt_cem_t cd;
    gbt_can_frame_t f;
    static const uint8_t bem[4] = { 0xF0U, 0xF0U, 0xF1U, 0xFCU };
    static const uint8_t cem[4] = { 0xFCU, 0xF0U, 0xC4U, 0xF0U };

    memset(&b, 0, sizeof b);
    b.code[GBT_BEM_CCS] = GBT_CODE_01;          /* SPN3905 CCS timeout */
    CHECK(gbt_encode_bem(&b, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.id, 0x081E56F4u);
    CHECK(eq(f.data, bem, 4U));                 /* reserved SPN3908 (4.3, 6 bits) = 1s */
    CHECK(gbt_decode_bem(&f, &bd) == GBT_CODEC_OK);
    CHECK_EQ_U(bd.code[GBT_BEM_CCS], GBT_CODE_01);

    memset(&c, 0, sizeof c);
    c.code[GBT_CEM_BCL] = GBT_CODE_01;          /* SPN3925 BCL timeout */
    CHECK(gbt_encode_cem(&c, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.id, 0x081FF456u);
    CHECK(eq(f.data, cem, 4U));                 /* reserved SPN3930 (4.5, 4 bits) = 1s */
    CHECK(gbt_decode_cem(&f, &cd) == GBT_CODEC_OK);
    CHECK_EQ_U(cd.code[GBT_CEM_BCL], GBT_CODE_01);
    CHECK_EQ_U(cd.code[GBT_CEM_BSM], GBT_CODE_00);

    f.data[0] = 0xFFU;                          /* SPN3921 = 11 */
    CHECK(gbt_decode_cem(&f, &cd) == GBT_CODEC_E_VALUE);
}

static void test_csd_bcp_bcs(void)
{
    gbt_csd_t csd = { 120U, 255U, 0x01020304u };
    gbt_csd_t cd;
    gbt_bcp_t bcp = { 365U, 1500U, 600U, 4200U, 105U, 500U, 3500U };
    gbt_bcp_t pd;
    gbt_bcs_t bcs = { 3800U, 3000U, 365U, 2U, 50U, 90U };
    gbt_bcs_t sd;
    gbt_can_frame_t f;
    uint8_t buf13[GBT_LEN_BCP];
    uint8_t buf9[GBT_LEN_BCS];
    static const uint8_t g_csd[8] = { 0x78U, 0x00U, 0xFFU, 0x00U, 0x04U, 0x03U, 0x02U, 0x01U };
    static const uint8_t g_bcp[13] = { 0x6DU, 0x01U, 0xDCU, 0x05U, 0x58U, 0x02U, 0x68U, 0x10U,
                                       0x69U, 0xF4U, 0x01U, 0xACU, 0x0DU };
    static const uint8_t g_bcs[9] = { 0xD8U, 0x0EU, 0xB8U, 0x0BU, 0x6DU, 0x21U, 0x32U, 0x5AU, 0x00U };

    CHECK(gbt_encode_csd(&csd, &f) == GBT_CODEC_OK);
    CHECK_EQ_U(f.id, 0x181DF456u);
    CHECK(eq(f.data, g_csd, 8U));
    CHECK(gbt_decode_csd(&f, &cd) == GBT_CODEC_OK);
    CHECK_EQ_U(gbt_csd_charger_number(cd.charger_number_raw), 0x01020304u);  /* P-K5-02 offset 0 */

    CHECK(gbt_encode_bcp_payload(&bcp, buf13, sizeof buf13) == GBT_CODEC_OK);
    CHECK(eq(buf13, g_bcp, 13U));
    CHECK(gbt_decode_bcp_payload(buf13, sizeof buf13, &pd) == GBT_CODEC_OK);
    CHECK_EQ_U(pd.max_current_raw, 1500U);      /* P-K5-01: -250.0 A */
    CHECK_EQ_U(pd.present_voltage_raw, 3500U);  /* P-K5-03: 350.0 V */
    bcp.max_current_raw = 5000U;
    CHECK(gbt_encode_bcp_payload(&bcp, buf13, sizeof buf13) == GBT_CODEC_OK);
    CHECK(gbt_decode_bcp_payload(buf13, sizeof buf13, &pd) == GBT_CODEC_E_RANGE);
    CHECK(gbt_decode_bcp_payload(buf13, 12U, &pd) == GBT_CODEC_E_LENGTH);

    CHECK(gbt_encode_bcs_payload(&bcs, buf9, sizeof buf9) == GBT_CODEC_OK);
    CHECK(eq(buf9, g_bcs, 9U));                 /* SPN3077 = 365 | 2 << 12 = 0x216D (K-3) */
    CHECK(gbt_decode_bcs_payload(buf9, sizeof buf9, &sd) == GBT_CODEC_OK);
    CHECK_EQ_U(sd.max_cell_v_raw, 365U);
    CHECK_EQ_U(sd.max_cell_group, 2U);
    CHECK_EQ_U(sd.remaining_min, 90U);
    CHECK(gbt_decode_bcs_payload(buf9, 8U, &sd) == GBT_CODEC_E_LENGTH);
    CHECK(gbt_encode_bcs_payload(&bcs, buf9, 8U) == GBT_CODEC_E_PARAM);
}

int main(void)
{
    test_ccs();
    test_bsm();
    test_bst_cst();
    test_bem_cem();
    test_csd_bcp_bcs();
    return TEST_SUMMARY("test_codec_ext");
}
