/*
 * protocol/gbt_codec_ext.c - see gbt_codec_ext.h.
 * Field positions: docs/sources/01.2_PROTOCOL_TABLES.md §3.5, §3.11-§3.13,
 * §3.17-§3.18, §3.20-§3.22. All bit positions go through gbt_bits (K-3).
 */
#include "protocol/gbt_codec_ext.h"
#include "protocol/gbt_bits.h"
#include "protocol/gbt_codec_internal.h"
#include "protocol/gbt_id.h"
#include "protocol/gbt_provisional.h"

#include <string.h>

typedef struct {
    uint8_t byte;   /* start byte (1-based) */
    uint8_t bit;    /* start bit (1-based) */
} pos_t;

/* BSM SPN3090..3096 (Table 20) */
static const pos_t k_bsm_pos[GBT_BSM_STATUS_COUNT] = {
    { 6U, 1U }, { 6U, 3U }, { 6U, 5U }, { 6U, 7U }, { 7U, 1U }, { 7U, 3U }, { 7U, 5U }
};
/* BEM SPN3901..3907 (Table 28); SPN3908 reserved (4.3, 6 bits) stays all 1s */
static const pos_t k_bem_pos[GBT_BEM_FIELDS] = {
    { 1U, 1U }, { 1U, 3U }, { 2U, 1U }, { 2U, 3U }, { 3U, 1U }, { 3U, 3U }, { 4U, 1U }
};
/* CEM SPN3921..3928 (Table 29); SPN3930 reserved (4.5, 4 bits) stays all 1s */
static const pos_t k_cem_pos[GBT_CEM_FIELDS] = {
    { 1U, 1U }, { 2U, 1U }, { 2U, 3U }, { 3U, 1U }, { 3U, 3U }, { 3U, 5U }, { 4U, 1U }, { 4U, 3U }
};

static void fill_ones(gbt_can_frame_t *f, uint8_t len)
{
    memset(f->data, 0xFF, len);   /* §8.8 undefined bits = 1 */
}

static uint8_t get2(const uint8_t *buf, pos_t p)
{
    return (uint8_t)gbt_bits_get(buf, p.byte, p.bit, 2U);
}

static void set2(uint8_t *buf, pos_t p, uint8_t code)
{
    gbt_bits_set(buf, p.byte, p.bit, 2U, (uint16_t)(code & 0x03U));
}

/* ---- CCS (Table 19) ---- */
gbt_codec_status_t gbt_encode_ccs(const gbt_ccs_t *m, gbt_can_frame_t *f)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_begin(GBT_MSG_CCS, (uint8_t)GBT_LEN_CCS, f);
    if (st == GBT_CODEC_OK) {
        fill_ones(f, (uint8_t)GBT_LEN_CCS);
        gbt_codec_put_u16(&f->data[0], m->voltage_raw);
        gbt_codec_put_u16(&f->data[2], m->current_raw);
        gbt_codec_put_u16(&f->data[4], m->charge_time_min);
        gbt_bits_set(f->data, 7U, 1U, 2U, (uint16_t)(m->permit & 0x03U));
    }
    return st;
}

gbt_codec_status_t gbt_decode_ccs(const gbt_can_frame_t *f, gbt_ccs_t *m)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_check(f, GBT_MSG_CCS, (uint8_t)GBT_LEN_CCS);
    if (st != GBT_CODEC_OK) {
        return st;
    }
    m->voltage_raw = gbt_codec_get_u16(&f->data[0]);
    m->current_raw = gbt_codec_get_u16(&f->data[2]);
    m->charge_time_min = gbt_codec_get_u16(&f->data[4]);
    m->permit = (uint8_t)gbt_bits_get(f->data, 7U, 1U, 2U);
    if (gbt_current_raw_is_valid(m->current_raw) == 0U) {
        return GBT_CODEC_E_RANGE;
    }
    if (m->permit > GBT_CODE_01) {
        return GBT_CODEC_E_VALUE;
    }
    return GBT_CODEC_OK;
}

/* ---- BSM (Table 20) ---- */
gbt_codec_status_t gbt_encode_bsm(const gbt_bsm_t *m, gbt_can_frame_t *f)
{
    gbt_codec_status_t st;
    unsigned i;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_begin(GBT_MSG_BSM, (uint8_t)GBT_LEN_BSM, f);
    if (st == GBT_CODEC_OK) {
        fill_ones(f, (uint8_t)GBT_LEN_BSM);
        f->data[0] = m->max_cell_v_index_raw;
        f->data[1] = m->max_temp_raw;
        f->data[2] = m->max_temp_point_raw;
        f->data[3] = m->min_temp_raw;
        f->data[4] = m->min_temp_point_raw;
        for (i = 0U; i < GBT_BSM_STATUS_COUNT; i++) {
            set2(f->data, k_bsm_pos[i], m->status[i]);
        }
    }
    return st;
}

gbt_codec_status_t gbt_decode_bsm(const gbt_can_frame_t *f, gbt_bsm_t *m)
{
    gbt_codec_status_t st;
    unsigned i;
    uint8_t bad = 0U;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_check(f, GBT_MSG_BSM, (uint8_t)GBT_LEN_BSM);
    if (st != GBT_CODEC_OK) {
        return st;
    }
    m->max_cell_v_index_raw = f->data[0];
    m->max_temp_raw = f->data[1];
    m->max_temp_point_raw = f->data[2];
    m->min_temp_raw = f->data[3];
    m->min_temp_point_raw = f->data[4];
    for (i = 0U; i < GBT_BSM_STATUS_COUNT; i++) {
        m->status[i] = get2(f->data, k_bsm_pos[i]);
        if (m->status[i] > GBT_CODE_10) {
            bad = 1U;
        }
    }
    if (m->status[GBT_BSM_IDX_CHARGE_PERMIT] > GBT_CODE_01) {
        bad = 1U;                       /* SPN3096 defines only 00 and 01 */
    }
    return (bad != 0U) ? GBT_CODEC_E_VALUE : GBT_CODEC_OK;
}

/* ---- BST / CST (Tables 24, 25) ---- */
static pos_t fault_pos(unsigned k)
{
    pos_t p;
    unsigned bit0 = 2U * k;            /* 0-based bit inside SPN3512/3522 (bytes 2-3) */
    p.byte = (uint8_t)(2U + bit0 / 8U);
    p.bit = (uint8_t)(bit0 % 8U + 1U);
    return p;
}

static gbt_codec_status_t encode_stop(gbt_msg_t msg, const gbt_stop_t *m, gbt_can_frame_t *f)
{
    gbt_codec_status_t st;
    unsigned i;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_begin(msg, (uint8_t)GBT_LEN_BST, f);
    if (st == GBT_CODEC_OK) {
        fill_ones(f, (uint8_t)GBT_LEN_BST);
        for (i = 0U; i < GBT_STOP_REASON_GROUPS; i++) {
            pos_t p;
            p.byte = 1U;
            p.bit = (uint8_t)(2U * i + 1U);
            set2(f->data, p, m->reason[i]);
        }
        for (i = 0U; i < GBT_STOP_FAULT_GROUPS; i++) {
            set2(f->data, fault_pos(i), m->fault[i]);
        }
        for (i = 0U; i < GBT_STOP_ERROR_GROUPS; i++) {
            pos_t p;
            p.byte = 4U;
            p.bit = (uint8_t)(2U * i + 1U);
            set2(f->data, p, m->error[i]);
        }
    }
    return st;
}

static gbt_codec_status_t decode_stop(gbt_msg_t msg, const gbt_can_frame_t *f, gbt_stop_t *m)
{
    gbt_codec_status_t st;
    unsigned i;
    uint8_t bad = 0U;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_check(f, msg, (uint8_t)GBT_LEN_BST);
    if (st != GBT_CODEC_OK) {
        return st;
    }
    for (i = 0U; i < GBT_STOP_REASON_GROUPS; i++) {
        pos_t p;
        p.byte = 1U;
        p.bit = (uint8_t)(2U * i + 1U);
        m->reason[i] = get2(f->data, p);
        bad = (uint8_t)(bad | (m->reason[i] > GBT_CODE_10));
    }
    for (i = 0U; i < GBT_STOP_FAULT_GROUPS; i++) {
        m->fault[i] = get2(f->data, fault_pos(i));
        bad = (uint8_t)(bad | (m->fault[i] > GBT_CODE_10));
    }
    for (i = 0U; i < GBT_STOP_ERROR_GROUPS; i++) {
        pos_t p;
        p.byte = 4U;
        p.bit = (uint8_t)(2U * i + 1U);
        m->error[i] = get2(f->data, p);
        bad = (uint8_t)(bad | (m->error[i] > GBT_CODE_10));
    }
    return (bad != 0U) ? GBT_CODEC_E_VALUE : GBT_CODEC_OK;
}

gbt_codec_status_t gbt_encode_bst(const gbt_stop_t *m, gbt_can_frame_t *f) { return encode_stop(GBT_MSG_BST, m, f); }
gbt_codec_status_t gbt_decode_bst(const gbt_can_frame_t *f, gbt_stop_t *m) { return decode_stop(GBT_MSG_BST, f, m); }
gbt_codec_status_t gbt_encode_cst(const gbt_stop_t *m, gbt_can_frame_t *f) { return encode_stop(GBT_MSG_CST, m, f); }
gbt_codec_status_t gbt_decode_cst(const gbt_can_frame_t *f, gbt_stop_t *m) { return decode_stop(GBT_MSG_CST, f, m); }

/* ---- BEM / CEM (Tables 28, 29) ---- */
static gbt_codec_status_t encode_codes(gbt_msg_t msg, const pos_t *pos, unsigned n,
                                       const uint8_t *code, gbt_can_frame_t *f)
{
    gbt_codec_status_t st = gbt_codec_frame_begin(msg, (uint8_t)GBT_LEN_BEM, f);
    unsigned i;
    if (st == GBT_CODEC_OK) {
        fill_ones(f, (uint8_t)GBT_LEN_BEM);   /* reserved SPN3908/3930 stay 1s */
        for (i = 0U; i < n; i++) {
            set2(f->data, pos[i], code[i]);
        }
    }
    return st;
}

static gbt_codec_status_t decode_codes(gbt_msg_t msg, const pos_t *pos, unsigned n,
                                       const gbt_can_frame_t *f, uint8_t *code)
{
    gbt_codec_status_t st = gbt_codec_frame_check(f, msg, (uint8_t)GBT_LEN_BEM);
    unsigned i;
    uint8_t bad = 0U;
    if (st != GBT_CODEC_OK) {
        return st;
    }
    for (i = 0U; i < n; i++) {
        code[i] = get2(f->data, pos[i]);
        bad = (uint8_t)(bad | (code[i] > GBT_CODE_10));
    }
    return (bad != 0U) ? GBT_CODEC_E_VALUE : GBT_CODEC_OK;
}

gbt_codec_status_t gbt_encode_bem(const gbt_bem_t *m, gbt_can_frame_t *f)
{
    return (m == NULL) ? GBT_CODEC_E_PARAM
                       : encode_codes(GBT_MSG_BEM, k_bem_pos, GBT_BEM_FIELDS, m->code, f);
}

gbt_codec_status_t gbt_decode_bem(const gbt_can_frame_t *f, gbt_bem_t *m)
{
    return (m == NULL) ? GBT_CODEC_E_PARAM
                       : decode_codes(GBT_MSG_BEM, k_bem_pos, GBT_BEM_FIELDS, f, m->code);
}

gbt_codec_status_t gbt_encode_cem(const gbt_cem_t *m, gbt_can_frame_t *f)
{
    return (m == NULL) ? GBT_CODEC_E_PARAM
                       : encode_codes(GBT_MSG_CEM, k_cem_pos, GBT_CEM_FIELDS, m->code, f);
}

gbt_codec_status_t gbt_decode_cem(const gbt_can_frame_t *f, gbt_cem_t *m)
{
    return (m == NULL) ? GBT_CODEC_E_PARAM
                       : decode_codes(GBT_MSG_CEM, k_cem_pos, GBT_CEM_FIELDS, f, m->code);
}

/* ---- CSD (Table 27) ---- */
gbt_codec_status_t gbt_encode_csd(const gbt_csd_t *m, gbt_can_frame_t *f)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_begin(GBT_MSG_CSD, (uint8_t)GBT_LEN_CSD, f);
    if (st == GBT_CODEC_OK) {
        gbt_codec_put_u16(&f->data[0], m->charge_time_min);
        gbt_codec_put_u16(&f->data[2], m->energy_raw);
        gbt_codec_put_u32(&f->data[4], m->charger_number_raw);
    }
    return st;
}

gbt_codec_status_t gbt_decode_csd(const gbt_can_frame_t *f, gbt_csd_t *m)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_check(f, GBT_MSG_CSD, (uint8_t)GBT_LEN_CSD);
    if (st == GBT_CODEC_OK) {
        m->charge_time_min = gbt_codec_get_u16(&f->data[0]);
        m->energy_raw = gbt_codec_get_u16(&f->data[2]);
        m->charger_number_raw = gbt_codec_get_u32(&f->data[4]);
    }
    return st;
}

uint32_t gbt_csd_charger_number(uint32_t raw)
{
    return (uint32_t)(raw + GBT_PROV_K5_SPN3613_OFFSET);   /* P-K5-02 PROVISIONAL */
}

/* ---- BCP payload (Table 12) ---- */
gbt_codec_status_t gbt_encode_bcp_payload(const gbt_bcp_t *m, uint8_t *buf, size_t buf_len)
{
    if (m == NULL || buf == NULL || buf_len != GBT_LEN_BCP) {
        return GBT_CODEC_E_PARAM;
    }
    gbt_codec_put_u16(&buf[0], m->max_cell_v_raw);       /* bytes 1-2   SPN2816 */
    gbt_codec_put_u16(&buf[2], m->max_current_raw);      /* bytes 3-4   SPN2817 */
    gbt_codec_put_u16(&buf[4], m->energy_raw);           /* bytes 5-6   SPN2818 */
    gbt_codec_put_u16(&buf[6], m->max_voltage_raw);      /* bytes 7-8   SPN2819 */
    buf[8] = m->max_temp_raw;                            /* byte 9      SPN2820 */
    gbt_codec_put_u16(&buf[9], m->soc_raw);              /* bytes 10-11 SPN2821 */
    gbt_codec_put_u16(&buf[11], m->present_voltage_raw); /* bytes 12-13 SPN2822 */
    return GBT_CODEC_OK;
}

gbt_codec_status_t gbt_decode_bcp_payload(const uint8_t *buf, size_t len, gbt_bcp_t *m)
{
    if (buf == NULL || m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    if (len != GBT_LEN_BCP) {
        return GBT_CODEC_E_LENGTH;
    }
    m->max_cell_v_raw = gbt_codec_get_u16(&buf[0]);
    m->max_current_raw = gbt_codec_get_u16(&buf[2]);
    m->energy_raw = gbt_codec_get_u16(&buf[4]);
    m->max_voltage_raw = gbt_codec_get_u16(&buf[6]);
    m->max_temp_raw = buf[8];
    m->soc_raw = gbt_codec_get_u16(&buf[9]);
    m->present_voltage_raw = gbt_codec_get_u16(&buf[11]);
    return (gbt_current_raw_is_valid(m->max_current_raw) == 0U) ? GBT_CODEC_E_RANGE : GBT_CODEC_OK;
}

/* ---- BCS payload (Table 18) ---- */
gbt_codec_status_t gbt_encode_bcs_payload(const gbt_bcs_t *m, uint8_t *buf, size_t buf_len)
{
    if (m == NULL || buf == NULL || buf_len != GBT_LEN_BCS) {
        return GBT_CODEC_E_PARAM;
    }
    gbt_codec_put_u16(&buf[0], m->voltage_raw);          /* bytes 1-2 SPN3075 */
    gbt_codec_put_u16(&buf[2], m->current_raw);          /* bytes 3-4 SPN3076 */
    gbt_bits_set(buf, 5U, 1U, 12U, m->max_cell_v_raw);   /* SPN3077 bits 1-12  (K-3) */
    gbt_bits_set(buf, 6U, 5U, 4U, m->max_cell_group);    /* SPN3077 bits 13-16 (K-3) */
    buf[6] = m->soc;                                     /* byte 7 SPN3078 */
    gbt_codec_put_u16(&buf[7], m->remaining_min);        /* bytes 8-9 SPN3079 */
    return GBT_CODEC_OK;
}

gbt_codec_status_t gbt_decode_bcs_payload(const uint8_t *buf, size_t len, gbt_bcs_t *m)
{
    if (buf == NULL || m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    if (len != GBT_LEN_BCS) {
        return GBT_CODEC_E_LENGTH;
    }
    m->voltage_raw = gbt_codec_get_u16(&buf[0]);
    m->current_raw = gbt_codec_get_u16(&buf[2]);
    m->max_cell_v_raw = gbt_bits_get(buf, 5U, 1U, 12U);
    m->max_cell_group = (uint8_t)gbt_bits_get(buf, 6U, 5U, 4U);
    m->soc = buf[6];
    m->remaining_min = gbt_codec_get_u16(&buf[7]);
    return (gbt_current_raw_is_valid(m->current_raw) == 0U) ? GBT_CODEC_E_RANGE : GBT_CODEC_OK;
}
