/*
 * protocol/gbt_node_internal.h - shared between gbt_node.c, gbt_charger_sm.c,
 * gbt_vehicle_sm.c and gbt_safety.c. Not part of the public API.
 */
#ifndef GBT_NODE_INTERNAL_H
#define GBT_NODE_INTERNAL_H

#include "protocol/gbt_node.h"

/* ---- Step codes: document-local labels of 01.2_STATE_MACHINES.md ---- */
#define GBT_STEP_IDLE    0u    /* C00/C01, V00/V01 */
#define GBT_STEP_C02     2u    /* CHM, wait BHM */
#define GBT_STEP_C03     3u    /* self-check */
#define GBT_STEP_C04     4u    /* CRM 00, wait BRM */
#define GBT_STEP_C05     5u    /* CRM AA, wait BCP */
#define GBT_STEP_C07     7u    /* CML, wait BRO / BRO AA */
#define GBT_STEP_C08     8u    /* CRO 00 */
#define GBT_STEP_C09     9u    /* CRO AA, wait BCL */
#define GBT_STEP_C10     10u   /* charging */
#define GBT_STEP_C10P    11u   /* charging, output inhibited by SPN3096 = 00 (TC29) */
#define GBT_STEP_C11     12u   /* CST, wait BST (charger-initiated) */
#define GBT_STEP_C12     13u   /* CST, wait BSD */
#define GBT_STEP_V02     22u   /* wait CHM */
#define GBT_STEP_V03     23u   /* BHM, wait CRM 00 */
#define GBT_STEP_V05     25u   /* BRM, wait CRM AA */
#define GBT_STEP_V06     26u   /* BCP, wait CML */
#define GBT_STEP_V09     29u   /* BRO AA, wait CRO / CRO AA */
#define GBT_STEP_V10     30u   /* BCL + BCS, wait CCS */
#define GBT_STEP_V11     31u   /* charging */
#define GBT_STEP_V12     32u   /* BST */
#define GBT_STEP_V13     33u   /* BSD, wait CSD */
#define GBT_STEP_TO      40u   /* C-TO / V-TO: sending CEM / BEM (K-6A) */
#define GBT_STEP_END     41u
#define GBT_STEP_EXIT    42u

/* ---- Timeout indices (TO-01..TO-25 -> 0..24); durations VERIFIED_GBT (01.2_TIMEOUT_TABLE.md) ---- */
#define GBT_TO(n) ((uint8_t)((n) - 1U))
extern const uint32_t gbt_to_duration_ms[GBT_NODE_TIMER_SLOTS];

/* ---- Message periods (Tables 3-7) ---- */
uint32_t gbt_node_period_ms(gbt_msg_t msg);

/* ---- Shared helpers (gbt_node.c) ---- */
void gbt_node_tx_enable(gbt_node_t *n, gbt_msg_t msg);      /* first frame at next tick */
void gbt_node_tx_disable(gbt_node_t *n, gbt_msg_t msg);
void gbt_node_tx_disable_all(gbt_node_t *n);
void gbt_node_timer_start(gbt_node_t *n, uint8_t to_index);
void gbt_node_timer_stop(gbt_node_t *n, uint8_t to_index);
uint8_t gbt_node_timer_expired(const gbt_node_t *n, uint8_t to_index);
void gbt_node_set_step(gbt_node_t *n, uint8_t step);
void gbt_node_push(gbt_node_t *n, const gbt_can_frame_t *f);
/* Enter K-6A timeout handling with the given BEM/CEM field index. */
void gbt_node_enter_timeout(gbt_node_t *n, uint8_t field);
/* Peer reported a timeout (BEM/CEM received): inhibit, stop, EXIT (K-6A rule 5). */
void gbt_node_peer_error(gbt_node_t *n);

/* ---- Safety (gbt_safety.c) ---- */
#define GBT_SAFE_REASON_VEHICLE_STOP 1u   /* T-1 */
#define GBT_SAFE_REASON_REQUEST      2u   /* T-2 */
#define GBT_SAFE_REASON_CURRENT      3u   /* T-3a / T-3b (ERR-6) */
#define GBT_SAFE_REASON_VOLTAGE_MIN  4u   /* T-3c */
#define GBT_SAFE_REASON_TIMEOUT      5u   /* T-4 */
#define GBT_SAFE_REASON_OVERFLOW     6u   /* T-5 */
#define GBT_SAFE_REASON_OUTPUT_FAIL  7u   /* ND-7 */
#define GBT_SAFE_REASON_PERMIT_OFF   8u   /* TC29 SPN3096 = 00 (D-08) */
#define GBT_SAFE_REASON_SHUTDOWN     9u
/* DISABLE_OUTPUT: latch inhibit, command disabled/0/0, new seq (TX gate PENDING). */
void gbt_safety_disable(gbt_node_t *n, uint8_t reason);
/* ZERO_CURRENT (SAF-3): current 0, enabled/voltage unchanged, no latch. */
void gbt_safety_zero_current(gbt_node_t *n);
/* PROT-7 setpoint from a valid BCL; no effect while inhibited. */
void gbt_safety_apply_bcl(gbt_node_t *n, const gbt_bcl_t *bcl);
/* 1 if BCL voltage demand is below the charger minimum (T-3c, PT L426). */
uint8_t gbt_safety_voltage_below_min(const gbt_node_t *n, const gbt_bcl_t *bcl);

/* ---- Role state machines ---- */
void gbt_csm_start(gbt_node_t *n);
void gbt_csm_on_msg(gbt_node_t *n, gbt_msg_t msg, const gbt_can_frame_t *f);
void gbt_csm_on_tp(gbt_node_t *n, uint32_t pgn, const uint8_t *buf, uint16_t len);
void gbt_csm_tick(gbt_node_t *n);
void gbt_csm_stop(gbt_node_t *n, uint8_t safe_reason);       /* charger-initiated stop (CST) */
uint8_t gbt_csm_build(gbt_node_t *n, gbt_msg_t msg, gbt_can_frame_t *f);
uint8_t gbt_csm_accept_tp(uint32_t pgn, uint16_t len);

void gbt_vsm_start(gbt_node_t *n);
void gbt_vsm_on_msg(gbt_node_t *n, gbt_msg_t msg, const gbt_can_frame_t *f);
void gbt_vsm_tick(gbt_node_t *n);
void gbt_vsm_stop(gbt_node_t *n, uint8_t reason);            /* vehicle-initiated stop (BST) */
uint8_t gbt_vsm_build(gbt_node_t *n, gbt_msg_t msg, gbt_can_frame_t *f);
/* TP payload for a vehicle TP message; returns length or 0 */
uint16_t gbt_vsm_tp_payload(gbt_node_t *n, gbt_msg_t msg, uint8_t *buf);

#endif /* GBT_NODE_INTERNAL_H */
