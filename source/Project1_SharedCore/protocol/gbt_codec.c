/*
 * protocol/gbt_codec.c - see gbt_codec.h.
 * Field positions: docs/sources/01.2_PROTOCOL_TABLES.md §3 (Tables 8-11, 14-17, 26).
 */
#include "protocol/gbt_codec.h"
#include "protocol/gbt_codec_internal.h"
#include "protocol/gbt_id.h"
#include "protocol/gbt_provisional.h"

#include <string.h>

const uint8_t gbt_chm_version_v1_1[3]   = { 0x01U, 0x01U, 0x00U };
const uint8_t gbt_brm_version_v1_10[3]  = { 0x01U, 0x01U, 0x00U };

/* ---- little-endian helpers (§5.3 low byte first) ---- */
void gbt_codec_put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFU);
    p[1] = (uint8_t)((v >> 8) & 0xFFU);
}

void gbt_codec_put_u24(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFFU);
    p[1] = (uint8_t)((v >> 8) & 0xFFU);
    p[2] = (uint8_t)((v >> 16) & 0xFFU);
}

void gbt_codec_put_u32(uint8_t *p, uint32_t v)
{
    gbt_codec_put_u24(p, v);
    p[3] = (uint8_t)((v >> 24) & 0xFFU);
}

uint16_t gbt_codec_get_u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8));
}

uint32_t gbt_codec_get_u24(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

uint32_t gbt_codec_get_u32(const uint8_t *p)
{
    return gbt_codec_get_u24(p) | ((uint32_t)p[3] << 24);
}

/* ---- optional fields (§8.7: unsent optional parameters are all 1s) ---- */
static void put_opt(uint8_t *dst, const uint8_t *src, size_t n, uint8_t present)
{
    if (present != 0U) {
        memcpy(dst, src, n);
    } else {
        memset(dst, 0xFF, n);
    }
}

static uint8_t get_opt(uint8_t *dst, const uint8_t *src, size_t n)
{
    size_t i;
    memcpy(dst, src, n);
    for (i = 0U; i < n; i++) {
        if (src[i] != 0xFFU) {
            return 1U;
        }
    }
    return 0U;   /* all 1s: treated as not sent (indistinguishable by §8.7) */
}

/* ---- frame helpers ---- */
gbt_codec_status_t gbt_codec_frame_begin(gbt_msg_t msg, uint8_t len, gbt_can_frame_t *f)
{
    uint32_t id;
    if (f == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    if (gbt_id_for_msg(msg, &id) != GBT_ID_OK) {
        return GBT_CODEC_E_PARAM;
    }
    f->id = id;
    f->dlc = len;                         /* §8.8: specified length, no padding */
    f->flags = (uint8_t)GBT_CAN_FLAG_EXT;
    memset(f->data, 0x00, sizeof f->data); /* bytes beyond DLC are not transmitted */
    return GBT_CODEC_OK;
}

gbt_codec_status_t gbt_codec_frame_check(const gbt_can_frame_t *f, gbt_msg_t msg, uint8_t len)
{
    gbt_msg_t got;
    if (f == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    if (f->flags != (uint8_t)GBT_CAN_FLAG_EXT || f->id > GBT_CAN_MAX_EXT_ID) {
        return GBT_CODEC_E_FORMAT;
    }
    if (gbt_id_classify(f->id, &got) != GBT_ID_OK || got != msg) {
        return GBT_CODEC_E_ID;
    }
    if (f->dlc != len) {
        return GBT_CODEC_E_LENGTH;
    }
    return GBT_CODEC_OK;
}

/* ---- conversions and range check ---- */
uint8_t gbt_current_raw_is_valid(uint16_t raw)
{
    if (raw > GBT_CURRENT_RAW_AT_0A) {
        return 0U;                                       /* > 0 A: outside §5.4 range */
    }
    if (raw == GBT_CURRENT_RAW_AT_0A) {
        return (uint8_t)GBT_PROV_UR2_MAX_ENDPOINT_VALID; /* 0 A endpoint, PROVISIONAL */
    }
    if (raw == 0U) {
        return (uint8_t)GBT_PROV_UR2_MIN_ENDPOINT_VALID; /* -400 A endpoint, PROVISIONAL */
    }
    return 1U;
}

gbt_codec_status_t gbt_current_raw_to_magnitude_ma(uint16_t raw, uint32_t *ma)
{
    if (ma == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    if (gbt_current_raw_is_valid(raw) == 0U) {
        return GBT_CODEC_E_RANGE;
    }
    /* physical = 0.1 A x raw - 400 A (negative = charging); magnitude in mA */
    *ma = (uint32_t)(GBT_CURRENT_RAW_AT_0A - (uint32_t)raw) * 100U;
    return GBT_CODEC_OK;
}

uint32_t gbt_voltage_0p1v_raw_to_mv(uint16_t raw)
{
    return (uint32_t)raw * 100U;
}

uint32_t gbt_voltage_0p01v_raw_to_mv(uint16_t raw)
{
    return (uint32_t)raw * 10U;
}

int16_t gbt_temp_raw_to_degc(uint8_t raw)
{
    return (int16_t)((int16_t)raw - 50);
}

/* ---- CHM (Table 8) ---- */
gbt_codec_status_t gbt_encode_chm(const gbt_chm_t *m, gbt_can_frame_t *f)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_begin(GBT_MSG_CHM, (uint8_t)GBT_LEN_CHM, f);
    if (st == GBT_CODEC_OK) {
        memcpy(&f->data[0], m->version, 3U);
    }
    return st;
}

gbt_codec_status_t gbt_decode_chm(const gbt_can_frame_t *f, gbt_chm_t *m)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_check(f, GBT_MSG_CHM, (uint8_t)GBT_LEN_CHM);
    if (st == GBT_CODEC_OK) {
        memcpy(m->version, &f->data[0], 3U);
    }
    return st;
}

/* ---- BHM (Table 9) ---- */
gbt_codec_status_t gbt_encode_bhm(const gbt_bhm_t *m, gbt_can_frame_t *f)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_begin(GBT_MSG_BHM, (uint8_t)GBT_LEN_BHM, f);
    if (st == GBT_CODEC_OK) {
        gbt_codec_put_u16(&f->data[0], m->insulation_voltage_raw);
    }
    return st;
}

gbt_codec_status_t gbt_decode_bhm(const gbt_can_frame_t *f, gbt_bhm_t *m)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_check(f, GBT_MSG_BHM, (uint8_t)GBT_LEN_BHM);
    if (st == GBT_CODEC_OK) {
        m->insulation_voltage_raw = gbt_codec_get_u16(&f->data[0]);
    }
    return st;
}

/* ---- CRM (Table 10) ---- */
gbt_codec_status_t gbt_encode_crm(const gbt_crm_t *m, gbt_can_frame_t *f)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_begin(GBT_MSG_CRM, (uint8_t)GBT_LEN_CRM, f);
    if (st == GBT_CODEC_OK) {
        f->data[0] = m->result;
        gbt_codec_put_u32(&f->data[1], m->charger_number);
        put_opt(&f->data[5], m->region, 3U, m->region_present);
    }
    return st;
}

gbt_codec_status_t gbt_decode_crm(const gbt_can_frame_t *f, gbt_crm_t *m)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_check(f, GBT_MSG_CRM, (uint8_t)GBT_LEN_CRM);
    if (st == GBT_CODEC_OK) {
        m->result = f->data[0];
        m->charger_number = gbt_codec_get_u32(&f->data[1]);
        m->region_present = get_opt(m->region, &f->data[5], 3U);
    }
    return st;
}

/* ---- BRM payload (Table 11), 49 bytes ---- */
gbt_codec_status_t gbt_encode_brm_payload(const gbt_brm_t *m, uint8_t *buf, size_t buf_len)
{
    uint8_t tmp[3];
    if (m == NULL || buf == NULL || buf_len != GBT_LEN_BRM) {
        return GBT_CODEC_E_PARAM;
    }
    if (m->charge_count_present != 0U && m->charge_count > 0xFFFFFFu) {
        return GBT_CODEC_E_PARAM;   /* SPN2572 is 3 bytes */
    }
    memcpy(&buf[0], m->version, 3U);                     /* bytes 1-3   */
    buf[3] = m->battery_type;                            /* byte 4      */
    gbt_codec_put_u16(&buf[4], m->rated_capacity_raw);             /* bytes 5-6   */
    gbt_codec_put_u16(&buf[6], m->rated_voltage_raw);              /* bytes 7-8   */
    put_opt(&buf[8], m->manufacturer, 4U, m->manufacturer_present);  /* 9-12  */
    put_opt(&buf[12], m->pack_serial, 4U, m->pack_serial_present);   /* 13-16 */
    tmp[0] = m->prod_year_raw;
    tmp[1] = m->prod_month;
    tmp[2] = m->prod_day;
    put_opt(&buf[16], tmp, 3U, m->prod_date_present);                /* 17-19 */
    if (m->charge_count_present != 0U) {
        gbt_codec_put_u24(&buf[19], m->charge_count);                          /* 20-22 */
    } else {
        memset(&buf[19], 0xFF, 3U);
    }
    put_opt(&buf[22], &m->ownership, 1U, m->ownership_present);      /* 23    */
    put_opt(&buf[23], &m->reserved, 1U, m->reserved_present);        /* 24    */
    put_opt(&buf[24], m->vin, 17U, m->vin_present);                  /* 25-41 */
    put_opt(&buf[41], m->sw_version, 8U, m->sw_version_present);     /* 42-49 */
    return GBT_CODEC_OK;
}

gbt_codec_status_t gbt_decode_brm_payload(const uint8_t *buf, size_t len, gbt_brm_t *m)
{
    uint8_t tmp[3];
    if (buf == NULL || m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    if (len != GBT_LEN_BRM) {
        return GBT_CODEC_E_LENGTH;
    }
    memcpy(m->version, &buf[0], 3U);
    m->battery_type = buf[3];
    m->rated_capacity_raw = gbt_codec_get_u16(&buf[4]);
    m->rated_voltage_raw = gbt_codec_get_u16(&buf[6]);
    m->manufacturer_present = get_opt(m->manufacturer, &buf[8], 4U);
    m->pack_serial_present = get_opt(m->pack_serial, &buf[12], 4U);
    m->prod_date_present = get_opt(tmp, &buf[16], 3U);
    m->prod_year_raw = tmp[0];
    m->prod_month = tmp[1];
    m->prod_day = tmp[2];
    m->charge_count_present = get_opt(tmp, &buf[19], 3U);
    m->charge_count = gbt_codec_get_u24(&buf[19]);
    m->ownership_present = get_opt(&m->ownership, &buf[22], 1U);
    m->reserved_present = get_opt(&m->reserved, &buf[23], 1U);
    m->vin_present = get_opt(m->vin, &buf[24], 17U);
    m->sw_version_present = get_opt(m->sw_version, &buf[41], 8U);
    return GBT_CODEC_OK;
}

/* ---- CML (Table 14) ---- */
gbt_codec_status_t gbt_encode_cml(const gbt_cml_t *m, gbt_can_frame_t *f)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_begin(GBT_MSG_CML, (uint8_t)GBT_LEN_CML, f);
    if (st == GBT_CODEC_OK) {
        gbt_codec_put_u16(&f->data[0], m->max_voltage_raw);
        gbt_codec_put_u16(&f->data[2], m->min_voltage_raw);
        gbt_codec_put_u16(&f->data[4], m->max_current_raw);
        gbt_codec_put_u16(&f->data[6], m->min_current_raw);
    }
    return st;
}

gbt_codec_status_t gbt_decode_cml(const gbt_can_frame_t *f, gbt_cml_t *m)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_check(f, GBT_MSG_CML, (uint8_t)GBT_LEN_CML);
    if (st == GBT_CODEC_OK) {
        m->max_voltage_raw = gbt_codec_get_u16(&f->data[0]);
        m->min_voltage_raw = gbt_codec_get_u16(&f->data[2]);
        m->max_current_raw = gbt_codec_get_u16(&f->data[4]);
        m->min_current_raw = gbt_codec_get_u16(&f->data[6]);
    }
    return st;
}

/* ---- BRO / CRO (Tables 15, 16) ---- */
gbt_codec_status_t gbt_encode_bro(const gbt_bro_t *m, gbt_can_frame_t *f)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_begin(GBT_MSG_BRO, (uint8_t)GBT_LEN_BRO, f);
    if (st == GBT_CODEC_OK) {
        f->data[0] = m->ready;
    }
    return st;
}

gbt_codec_status_t gbt_decode_bro(const gbt_can_frame_t *f, gbt_bro_t *m)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_check(f, GBT_MSG_BRO, (uint8_t)GBT_LEN_BRO);
    if (st == GBT_CODEC_OK) {
        m->ready = f->data[0];
    }
    return st;
}

gbt_codec_status_t gbt_encode_cro(const gbt_cro_t *m, gbt_can_frame_t *f)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_begin(GBT_MSG_CRO, (uint8_t)GBT_LEN_CRO, f);
    if (st == GBT_CODEC_OK) {
        f->data[0] = m->ready;
    }
    return st;
}

gbt_codec_status_t gbt_decode_cro(const gbt_can_frame_t *f, gbt_cro_t *m)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_check(f, GBT_MSG_CRO, (uint8_t)GBT_LEN_CRO);
    if (st == GBT_CODEC_OK) {
        m->ready = f->data[0];
    }
    return st;
}

/* ---- BCL (Table 17) ---- */
gbt_codec_status_t gbt_encode_bcl(const gbt_bcl_t *m, gbt_can_frame_t *f)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_begin(GBT_MSG_BCL, (uint8_t)GBT_LEN_BCL, f);
    if (st == GBT_CODEC_OK) {
        gbt_codec_put_u16(&f->data[0], m->voltage_demand_raw);   /* bytes 1-2 */
        gbt_codec_put_u16(&f->data[2], m->current_demand_raw);   /* bytes 3-4: byte 3 low, byte 4 high */
        f->data[4] = m->mode;                          /* byte 5 */
    }
    return st;
}

gbt_codec_status_t gbt_decode_bcl(const gbt_can_frame_t *f, gbt_bcl_t *m)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_check(f, GBT_MSG_BCL, (uint8_t)GBT_LEN_BCL);
    if (st != GBT_CODEC_OK) {
        return st;
    }
    m->voltage_demand_raw = gbt_codec_get_u16(&f->data[0]);
    m->current_demand_raw = gbt_codec_get_u16(&f->data[2]);
    m->mode = f->data[4];
    m->invalid_mask = 0U;
    if (gbt_current_raw_is_valid(m->current_demand_raw) == 0U) {
        m->invalid_mask = (uint8_t)(m->invalid_mask | GBT_BCL_INVALID_CURRENT);
    }
    if (m->mode != GBT_BCL_MODE_CV && m->mode != GBT_BCL_MODE_CC) {
        m->invalid_mask = (uint8_t)(m->invalid_mask | GBT_BCL_INVALID_MODE);
    }
    if ((m->invalid_mask & GBT_BCL_INVALID_CURRENT) != 0U) {
        return GBT_CODEC_E_RANGE;
    }
    if ((m->invalid_mask & GBT_BCL_INVALID_MODE) != 0U) {
        return GBT_CODEC_E_VALUE;
    }
    return GBT_CODEC_OK;
}

/* ---- BSD (Table 26) ---- */
gbt_codec_status_t gbt_encode_bsd(const gbt_bsd_t *m, gbt_can_frame_t *f)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_begin(GBT_MSG_BSD, (uint8_t)GBT_LEN_BSD, f);
    if (st == GBT_CODEC_OK) {
        f->data[0] = m->soc;
        gbt_codec_put_u16(&f->data[1], m->min_cell_v_raw);
        gbt_codec_put_u16(&f->data[3], m->max_cell_v_raw);
        f->data[5] = m->min_temp_raw;
        f->data[6] = m->max_temp_raw;
    }
    return st;
}

gbt_codec_status_t gbt_decode_bsd(const gbt_can_frame_t *f, gbt_bsd_t *m)
{
    gbt_codec_status_t st;
    if (m == NULL) {
        return GBT_CODEC_E_PARAM;
    }
    st = gbt_codec_frame_check(f, GBT_MSG_BSD, (uint8_t)GBT_LEN_BSD);
    if (st == GBT_CODEC_OK) {
        m->soc = f->data[0];
        m->min_cell_v_raw = gbt_codec_get_u16(&f->data[1]);
        m->max_cell_v_raw = gbt_codec_get_u16(&f->data[3]);
        m->min_temp_raw = f->data[5];
        m->max_temp_raw = f->data[6];
    }
    return st;
}
