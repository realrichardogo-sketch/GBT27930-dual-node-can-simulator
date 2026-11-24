/* protocol/gbt_id.c - see gbt_id.h. Table rows: docs/sources/01.2_PROTOCOL_TABLES.md §2. */
#include "protocol/gbt_id.h"

#include <stddef.h>

/*
 * Columns: name, PGN, PF, P, length, period ms, direction, TP.
 * Source: GB/T 27930-2023 Tables 3-7 as recorded in 01.2_PROTOCOL_TABLES.md L94-115.
 */
static const gbt_msg_def_t gbt_msg_table[GBT_MSG_COUNT] = {
    { "CHM", 9728U, 0x26U, 6U,  3U, 250U, GBT_DIR_C2V, 0U },  /* T3, p.7 */
    { "BHM", 9984U, 0x27U, 6U,  2U, 250U, GBT_DIR_V2C, 0U },  /* T3, p.7 */
    { "CRM",  256U, 0x01U, 6U,  8U, 250U, GBT_DIR_C2V, 0U },  /* T3, p.7 */
    { "BRM",  512U, 0x02U, 7U, 49U, 250U, GBT_DIR_V2C, 1U },  /* T3, p.7 */
    { "BCP", 1536U, 0x06U, 7U, 13U, 500U, GBT_DIR_V2C, 1U },  /* T4, p.8 */
    { "CML", 2048U, 0x08U, 6U,  8U, 250U, GBT_DIR_C2V, 0U },  /* T4, p.8 */
    { "BRO", 2304U, 0x09U, 4U,  1U, 250U, GBT_DIR_V2C, 0U },  /* T4, p.8 */
    { "CRO", 2560U, 0x0AU, 4U,  1U, 250U, GBT_DIR_C2V, 0U },  /* T4, p.8 */
    { "BCL", 4096U, 0x10U, 6U,  5U,  50U, GBT_DIR_V2C, 0U },  /* T5, p.8 */
    { "BCS", 4352U, 0x11U, 7U,  9U, 250U, GBT_DIR_V2C, 1U },  /* T5, p.8 */
    { "CCS", 4608U, 0x12U, 6U,  7U,  50U, GBT_DIR_C2V, 0U },  /* T5, p.8 */
    { "BSM", 4864U, 0x13U, 6U,  7U, 250U, GBT_DIR_V2C, 0U },  /* T5, p.8 */
    { "BST", 6400U, 0x19U, 4U,  4U,  10U, GBT_DIR_V2C, 0U },  /* T5, p.8 */
    { "CST", 6656U, 0x1AU, 4U,  4U,  10U, GBT_DIR_C2V, 0U },  /* T5, p.8 */
    { "BSD", 7168U, 0x1CU, 6U,  7U, 250U, GBT_DIR_V2C, 0U },  /* T6, p.9 */
    { "CSD", 7424U, 0x1DU, 6U,  8U, 250U, GBT_DIR_C2V, 0U },  /* T6, p.9 */
    { "BEM", 7680U, 0x1EU, 2U,  4U, 250U, GBT_DIR_V2C, 0U },  /* T7, p.9 */
    { "CEM", 7936U, 0x1FU, 2U,  4U, 250U, GBT_DIR_C2V, 0U }   /* T7, p.9 */
};

const gbt_msg_def_t *gbt_msg_def(gbt_msg_t msg)
{
    if ((unsigned)msg >= (unsigned)GBT_MSG_COUNT) {
        return NULL;
    }
    return &gbt_msg_table[msg];
}

gbt_id_status_t gbt_id_make(uint8_t p, uint8_t pf, uint8_t ps, uint8_t sa, uint32_t *id)
{
    if (id == NULL) {
        return GBT_ID_E_PARAM;
    }
    if (p > GBT_ID_MAX_PRIORITY) {
        return GBT_ID_E_RANGE;
    }
    /* EDP = 0 (bit 25) and DP = 0 (bit 24) per GB/T Table 1 notes 2-3. */
    *id = ((uint32_t)p << 26) | ((uint32_t)pf << 16) | ((uint32_t)ps << 8) | (uint32_t)sa;
    return GBT_ID_OK;
}

gbt_id_status_t gbt_id_parse(uint32_t id, gbt_id_fields_t *out)
{
    if (out == NULL) {
        return GBT_ID_E_PARAM;
    }
    if (id > 0x1FFFFFFFu) {
        return GBT_ID_E_RANGE;
    }
    out->p   = (uint8_t)((id >> 26) & 0x07u);
    out->edp = (uint8_t)((id >> 25) & 0x01u);
    out->dp  = (uint8_t)((id >> 24) & 0x01u);
    out->pf  = (uint8_t)((id >> 16) & 0xFFu);
    out->ps  = (uint8_t)((id >> 8) & 0xFFu);
    out->sa  = (uint8_t)(id & 0xFFu);
    return GBT_ID_OK;
}

static void gbt_dir_addresses(uint8_t dir, uint8_t *ps, uint8_t *sa)
{
    if (dir == GBT_DIR_C2V) {
        *sa = (uint8_t)GBT_ADDR_SECC;
        *ps = (uint8_t)GBT_ADDR_EVCC;
    } else {
        *sa = (uint8_t)GBT_ADDR_EVCC;
        *ps = (uint8_t)GBT_ADDR_SECC;
    }
}

gbt_id_status_t gbt_id_for_msg(gbt_msg_t msg, uint32_t *id)
{
    const gbt_msg_def_t *d = gbt_msg_def(msg);
    uint8_t ps;
    uint8_t sa;

    if (d == NULL || id == NULL) {
        return GBT_ID_E_PARAM;
    }
    gbt_dir_addresses(d->dir, &ps, &sa);
    return gbt_id_make(d->priority, d->pf, ps, sa, id);
}

gbt_id_status_t gbt_id_classify(uint32_t id, gbt_msg_t *msg)
{
    gbt_id_fields_t f;
    gbt_id_status_t st;
    unsigned i;

    if (msg == NULL) {
        return GBT_ID_E_PARAM;
    }
    st = gbt_id_parse(id, &f);
    if (st != GBT_ID_OK) {
        return st;
    }
    if (f.edp != 0U || f.dp != 0U) {
        return GBT_ID_E_FORMAT;
    }
    for (i = 0U; i < (unsigned)GBT_MSG_COUNT; i++) {
        if (gbt_msg_table[i].pf == f.pf) {
            uint8_t ps;
            uint8_t sa;
            gbt_dir_addresses(gbt_msg_table[i].dir, &ps, &sa);
            if (f.ps != ps || f.sa != sa) {
                return GBT_ID_E_ADDRESS;
            }
            *msg = (gbt_msg_t)i;
            return GBT_ID_OK;
        }
    }
    return GBT_ID_E_UNKNOWN_PGN;
}
