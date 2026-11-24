/*
 * tests/unit/test_id.c - 29-bit identifiers (M4, B1-10).
 * Expected values: docs/sources/01.2_PROTOCOL_TABLES.md L94-115 (derived IDs).
 */
#include "protocol/gbt_id.h"
#include "tests/support/test_assert.h"

#include <string.h>

typedef struct {
    gbt_msg_t   msg;
    const char *name;
    uint32_t    id;
    uint16_t    length;
} expect_t;

static const expect_t k_expect[GBT_MSG_COUNT] = {
    { GBT_MSG_CHM, "CHM", 0x1826F456u,  3U },
    { GBT_MSG_BHM, "BHM", 0x182756F4u,  2U },
    { GBT_MSG_CRM, "CRM", 0x1801F456u,  8U },
    { GBT_MSG_BRM, "BRM", 0x1C0256F4u, 49U },
    { GBT_MSG_BCP, "BCP", 0x1C0656F4u, 13U },
    { GBT_MSG_CML, "CML", 0x1808F456u,  8U },
    { GBT_MSG_BRO, "BRO", 0x100956F4u,  1U },
    { GBT_MSG_CRO, "CRO", 0x100AF456u,  1U },
    { GBT_MSG_BCL, "BCL", 0x181056F4u,  5U },
    { GBT_MSG_BCS, "BCS", 0x1C1156F4u,  9U },
    { GBT_MSG_CCS, "CCS", 0x1812F456u,  7U },
    { GBT_MSG_BSM, "BSM", 0x181356F4u,  7U },
    { GBT_MSG_BST, "BST", 0x101956F4u,  4U },
    { GBT_MSG_CST, "CST", 0x101AF456u,  4U },
    { GBT_MSG_BSD, "BSD", 0x181C56F4u,  7U },
    { GBT_MSG_CSD, "CSD", 0x181DF456u,  8U },
    { GBT_MSG_BEM, "BEM", 0x081E56F4u,  4U },
    { GBT_MSG_CEM, "CEM", 0x081FF456u,  4U }
};

static void test_all_lean_identifiers(void)
{
    unsigned i;
    CHECK_EQ_U(GBT_MSG_COUNT, 18U);
    for (i = 0U; i < (unsigned)GBT_MSG_COUNT; i++) {
        const gbt_msg_def_t *d = gbt_msg_def(k_expect[i].msg);
        uint32_t id = 0U;
        gbt_msg_t back = GBT_MSG_COUNT;
        CHECK(d != NULL);
        if (d == NULL) {
            continue;
        }
        CHECK(strcmp(d->name, k_expect[i].name) == 0);
        CHECK_EQ_U(d->length, k_expect[i].length);
        CHECK_EQ_U(d->tp, (k_expect[i].length > 8U) ? 1U : 0U);
        CHECK_EQ_U(d->pf, (d->pgn >> 8) & 0xFFU);          /* §7.4 */
        CHECK_EQ_U(d->pgn & 0xFFU, 0U);
        CHECK(gbt_id_for_msg(k_expect[i].msg, &id) == GBT_ID_OK);
        CHECK_EQ_U(id, k_expect[i].id);
        CHECK(gbt_id_classify(id, &back) == GBT_ID_OK);
        CHECK(back == k_expect[i].msg);
    }
    CHECK(gbt_msg_def(GBT_MSG_COUNT) == NULL);
}

static void test_make_parse_roundtrip(void)
{
    uint32_t p;
    uint32_t pf;
    uint32_t ps;
    uint32_t sa;
    unsigned bad = 0U;

    for (p = 0U; p <= 7U; p++) {
        for (pf = 0U; pf <= 255U; pf += 17U) {
            for (ps = 0U; ps <= 255U; ps += 51U) {
                for (sa = 0U; sa <= 255U; sa += 85U) {
                    uint32_t id = 0U;
                    gbt_id_fields_t f;
                    if (gbt_id_make((uint8_t)p, (uint8_t)pf, (uint8_t)ps, (uint8_t)sa, &id) != GBT_ID_OK ||
                        gbt_id_parse(id, &f) != GBT_ID_OK ||
                        f.p != p || f.pf != pf || f.ps != ps || f.sa != sa ||
                        f.edp != 0U || f.dp != 0U || id > 0x1FFFFFFFu) {
                        bad++;
                    }
                }
            }
        }
    }
    CHECK_EQ_U(bad, 0U);
}

static void test_errors(void)
{
    uint32_t id = 0U;
    gbt_id_fields_t f;
    gbt_msg_t m = GBT_MSG_COUNT;

    CHECK(gbt_id_make(8U, 0x26U, 0xF4U, 0x56U, &id) == GBT_ID_E_RANGE);
    CHECK(gbt_id_make(6U, 0x26U, 0xF4U, 0x56U, NULL) == GBT_ID_E_PARAM);
    CHECK(gbt_id_parse(0x20000000u, &f) == GBT_ID_E_RANGE);
    CHECK(gbt_id_parse(0x1826F456u, NULL) == GBT_ID_E_PARAM);
    CHECK(gbt_id_for_msg(GBT_MSG_COUNT, &id) == GBT_ID_E_PARAM);
    CHECK(gbt_id_classify(0x1826F456u, NULL) == GBT_ID_E_PARAM);

    CHECK(gbt_id_classify(0x20000000u, &m) == GBT_ID_E_RANGE);
    CHECK(gbt_id_classify(0x1826F456u | (1u << 25), &m) == GBT_ID_E_FORMAT);   /* EDP = 1 */
    CHECK(gbt_id_classify(0x1826F456u | (1u << 24), &m) == GBT_ID_E_FORMAT);   /* DP = 1 */
    CHECK(gbt_id_classify(0x1807F456u, &m) == GBT_ID_E_UNKNOWN_PGN);           /* CTS excluded */
    CHECK(gbt_id_classify(0x1C1556F4u, &m) == GBT_ID_E_UNKNOWN_PGN);           /* BMV excluded */
    CHECK(gbt_id_classify(0x1CEC56F4u, &m) == GBT_ID_E_UNKNOWN_PGN);           /* not Lean (TP is M8) */
    CHECK(gbt_id_classify(0x182656F4u, &m) == GBT_ID_E_ADDRESS);               /* CHM wrong direction */
    CHECK(gbt_id_classify(0x1826F400u, &m) == GBT_ID_E_ADDRESS);               /* wrong SA */
}

static void test_priority_not_used_for_identification(void)
{
    /* GB/T §8.2: nodes identify content by PGN. */
    gbt_msg_t m = GBT_MSG_COUNT;
    gbt_id_fields_t f;
    CHECK(gbt_id_classify(0x0026F456u, &m) == GBT_ID_OK);   /* CHM with P = 0 */
    CHECK(m == GBT_MSG_CHM);
    CHECK(gbt_id_parse(0x0026F456u, &f) == GBT_ID_OK);
    CHECK_EQ_U(f.p, 0U);
    CHECK(gbt_id_classify(0x1C1056F4u, &m) == GBT_ID_OK);   /* BCL with P = 7 */
    CHECK(m == GBT_MSG_BCL);
}

int main(void)
{
    test_all_lean_identifiers();
    test_make_parse_roundtrip();
    test_errors();
    test_priority_not_used_for_identification();
    return TEST_SUMMARY("test_id");
}
