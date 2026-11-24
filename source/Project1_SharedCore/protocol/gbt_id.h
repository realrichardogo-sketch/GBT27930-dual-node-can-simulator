/*
 * protocol/gbt_id.h - 29-bit CAN identifier construction and parsing (M4).
 *
 * Provenance (B1-10):
 *   ID = (P << 26) | (EDP(0) << 25) | (DP(0) << 24) | (PF << 16) | (PS << 8) | SA
 *   Field order and widths: GB/T 27930-2023 Table 1 (P 3, EDP 1, DP 1, PF 8,
 *   PS 8, SA 8); EDP = DP = 0 (Table 1 notes 2-3); PS = destination address,
 *   PDU1 (Table 1 note 5, §7.3); PGN second byte = PF (§7.4); addresses
 *   SECC 0x56 / EVCC 0xF4 (§7.6, Table 2).
 *   GB/T §7.1 delegates the bit assignment of the identifier to
 *   SAE J1939-21:2006. The formula is therefore DERIVED and corroborated
 *   (AUTOSAR R4.2.2 J1939Tp, which references J1939-21 2006-12; Lean Plan
 *   acceptance example CHM = 0x1826F456); it is not printed in GB/T.
 *   Derived identifiers: docs/sources/01.2_PROTOCOL_TABLES.md L94-115.
 *
 * Data-field bit numbering (K-3) is a separate, unresolved issue and is not
 * touched by this module.
 */
#ifndef GBT_ID_H
#define GBT_ID_H

#include <stdint.h>

#define GBT_ADDR_SECC 0x56u   /* charger, Table 2 */
#define GBT_ADDR_EVCC 0xF4u   /* vehicle, Table 2 */

#define GBT_ID_MAX_PRIORITY 7u

typedef enum {
    GBT_MSG_CHM = 0,
    GBT_MSG_BHM,
    GBT_MSG_CRM,
    GBT_MSG_BRM,
    GBT_MSG_BCP,
    GBT_MSG_CML,
    GBT_MSG_BRO,
    GBT_MSG_CRO,
    GBT_MSG_BCL,
    GBT_MSG_BCS,
    GBT_MSG_CCS,
    GBT_MSG_BSM,
    GBT_MSG_BST,
    GBT_MSG_CST,
    GBT_MSG_BSD,
    GBT_MSG_CSD,
    GBT_MSG_BEM,
    GBT_MSG_CEM,
    GBT_MSG_COUNT          /* 18 Lean messages; CTS, BMV, BMT, BSP excluded */
} gbt_msg_t;

#define GBT_DIR_C2V 0u     /* charger -> vehicle: SA 0x56, PS 0xF4 */
#define GBT_DIR_V2C 1u     /* vehicle -> charger: SA 0xF4, PS 0x56 */

typedef struct {
    char        name[4];   /* standard code, e.g. "CHM", NUL-terminated */
    uint32_t    pgn;       /* decimal PGN as printed in Tables 3-7 */
    uint8_t     pf;        /* PDU format = PGN second byte (§7.4) */
    uint8_t     priority;  /* P, Tables 3-7 */
    uint16_t    length;    /* message length in bytes, Tables 3-7 */
    uint16_t    period_ms; /* transmission period, Tables 3-7 */
    uint8_t     dir;       /* GBT_DIR_* */
    uint8_t     tp;        /* 1 if length > 8 (transport protocol, §7.5) */
} gbt_msg_def_t;

typedef struct {
    uint8_t p;
    uint8_t edp;
    uint8_t dp;
    uint8_t pf;
    uint8_t ps;
    uint8_t sa;
} gbt_id_fields_t;

typedef enum {
    GBT_ID_OK = 0,
    GBT_ID_E_PARAM,        /* NULL pointer or msg out of range */
    GBT_ID_E_RANGE,        /* priority > 7 or identifier > 29 bits */
    GBT_ID_E_FORMAT,       /* EDP or DP not 0 */
    GBT_ID_E_UNKNOWN_PGN,  /* PF is not one of the 18 Lean messages */
    GBT_ID_E_ADDRESS       /* PS/SA do not match the message direction */
} gbt_id_status_t;

const gbt_msg_def_t *gbt_msg_def(gbt_msg_t msg);   /* NULL if out of range */

gbt_id_status_t gbt_id_make(uint8_t p, uint8_t pf, uint8_t ps, uint8_t sa, uint32_t *id);
gbt_id_status_t gbt_id_parse(uint32_t id, gbt_id_fields_t *out);
gbt_id_status_t gbt_id_for_msg(gbt_msg_t msg, uint32_t *id);
/*
 * Identify a received identifier. Priority is reported by gbt_id_parse() but
 * not used for identification: GB/T §8.2 states that nodes identify a packet's
 * content by its PGN.
 */
gbt_id_status_t gbt_id_classify(uint32_t id, gbt_msg_t *msg);

#endif /* GBT_ID_H */
