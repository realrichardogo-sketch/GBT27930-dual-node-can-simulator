/*
 * protocol/gbt_node.h - protocol node public interface (C99).
 * Architecture v1.0 (STEP 07-A.1, Owner approved). Implemented in STEP 07-B by
 * gbt_node.c, gbt_charger_sm.c, gbt_vehicle_sm.c and gbt_safety.c.
 *
 * Dependencies: common/gbt_types.h and protocol-internal headers only.
 * The node never includes hal/hal.h or app/ headers and never calls the HAL.
 * The integration layer moves frames between the node and the HAL and
 * delivers output commands through the ND-1 HAL output sink.
 *
 * Attach / detach (integration layer, ND-1 lifecycle):
 *   gbt_port_<p>_init(h) -> output_if_make_sink(&backend, &sink)
 *   -> gbt_hal_bind_output_sink(h, &sink) -> gbt_hal_claim(h)
 *   -> gbt_node_init(n, &cfg) -> APPLY (initial disabled command, seq 1)
 *   ... integration loop ...
 *   -> gbt_node_shutdown(n) -> APPLY (final disabled command)
 *   -> gbt_hal_release(h) -> gbt_hal_unbind_output_sink(h) -> gbt_port_<p>_deinit(h)
 *   If the initial APPLY is refused, the node stays in output state FAILED
 *   and pops no NORMAL frames; the application keeps the node stopped.
 *
 * Integration loop (one node, main context; the HAL instance is CLAIMED):
 *     now = gbt_hal_now_ms(h);
 *     report RX overflow: gbt_node_on_rx_overflow(n, delta, now)      then APPLY
 *     while (gbt_hal_can_receive(h, &f) == GBT_HAL_OK)                 (bounded)
 *         { gbt_node_on_rx(n, &f, now);                                then APPLY }
 *     gbt_node_tick(n, now);                                           then APPLY
 *     SEND (TX ownership contract below)
 *   APPLY: gbt_node_get_output(n, &cmd); if cmd.seq changed:
 *          st = gbt_hal_output_apply(h, &cmd);
 *          gbt_node_confirm_output(n, cmd.seq, st == GBT_HAL_OK, now);
 *   APPLY runs immediately after EVERY gbt_node_* call that can change the
 *   command, so the output is set before any frame of the same event is sent.
 *   gbt_hal_output_apply -> sink is the ONLY path to the physical output.
 *
 * TX ownership contract (SEND):
 *   - gbt_node_pop_tx() copies the next frame into the caller's buffer and
 *     removes it from the node: from then on the integration layer owns it and
 *     the node never re-sends it.
 *   - The integration layer holds at most ONE popped frame ("pending"). It
 *     does not pop again while a frame is pending, which preserves order.
 *   - Before (re)sending the pending frame it calls gbt_node_tx_check():
 *     GBT_NODE_TX_SEND -> send; GBT_NODE_TX_HOLD -> keep, try next loop;
 *     GBT_NODE_TX_DISCARD -> drop it (counted by the node).
 *   - gbt_hal_can_send result: OK -> pending cleared (frame handed over);
 *     E_FULL -> frame NOT accepted, keep it pending and retry it first next
 *     loop (no duplicate: the HAL did not accept it, the node no longer has
 *     it); E_BUS_OFF, E_IO, E_NOT_READY, E_STATE, E_PARAM -> frame NOT
 *     accepted, discard it and count it in the integration layer (no retry, no bus-off recovery;
 *     periodic messages are regenerated at their next period; ND-3 PROVISIONAL).
 *
 * Contracts common to every function:
 *   - Execution context: main context only; not re-entrant for the same node;
 *     never called from an interrupt handler. Different nodes are independent.
 *   - Memory: no dynamic allocation; gbt_node_t is allocated by the caller
 *     (static or automatic) and must not be copied or moved after init.
 *   - Frame ownership: frames passed in are read during the call only (the
 *     node copies what it needs); frames passed out are copied into the
 *     caller's buffer.
 *   - Time: now_ms is a wrap-safe 32-bit millisecond count from the same clock
 *     for every call on the same node.
 *   - Errors: void functions never block or abort. Invalid arguments are
 *     ignored and counted in gbt_node_status_t.api_errors; an invalid
 *     configuration puts the node in GBT_NODE_PHASE_CONFIG_ERROR, where it
 *     produces no frames and a disabled output command.
 */
#ifndef GBT_NODE_H
#define GBT_NODE_H

#include <stdbool.h>
#include <stdint.h>
#include "common/gbt_types.h"
#include "protocol/gbt_codec_ext.h"
#include "protocol/gbt_id.h"
#include "protocol/gbt_outbox.h"
#include "protocol/gbt_timer.h"
#include "protocol/gbt_tp.h"

/* ---- Roles ---- */
#define GBT_NODE_ROLE_CHARGER 1u   /* SECC, source address 0x56 (GB/T Table 2) */
#define GBT_NODE_ROLE_VEHICLE 2u   /* EVCC, source address 0xF4 (GB/T Table 2) */

/* ---- Sizes (static memory) ---- */
#define GBT_NODE_TIMER_SLOTS     25u   /* indexed by canonical TO-01..TO-25 */

/* ---- Coarse phase reported in status (detailed step is protocol-private) ---- */
#define GBT_NODE_PHASE_IDLE          0u
#define GBT_NODE_PHASE_HANDSHAKE     1u
#define GBT_NODE_PHASE_CONFIG        2u
#define GBT_NODE_PHASE_CHARGING      3u
#define GBT_NODE_PHASE_STOPPING      4u
#define GBT_NODE_PHASE_END           5u   /* normal session end */
#define GBT_NODE_PHASE_TIMEOUT       6u   /* BEM/CEM timeout handling */
#define GBT_NODE_PHASE_EXIT          7u   /* exited; restart required */
#define GBT_NODE_PHASE_CONFIG_ERROR  8u

/* ---- Abstract stop reasons (wire encoding is a codec concern) ---- */
#define GBT_STOP_REASON_NONE        0u
#define GBT_STOP_REASON_REQUEST     1u   /* user / end condition */
#define GBT_STOP_REASON_FAULT       2u
#define GBT_STOP_REASON_SOC_TARGET  3u   /* vehicle: required SOC reached */

/* ---- Output state reported in status ---- */
#define GBT_NODE_OUT_ACCEPTED 0u
#define GBT_NODE_OUT_PENDING  1u   /* new command not yet confirmed (gbt_node_confirm_output) */
#define GBT_NODE_OUT_FAILED   2u   /* sink refused the command */

/* ---- Configuration (application-supplied project values) ---- */
typedef struct {
    uint32_t max_voltage_mv;
    uint32_t min_voltage_mv;
    uint32_t max_current_ma;
    uint32_t min_current_ma;
    uint32_t charger_number;          /* CRM SPN2561 */
} gbt_charger_params_t;

typedef struct {
    uint8_t  battery_type;            /* BRM SPN2566 code */
    uint32_t rated_capacity_mah;
    uint32_t rated_voltage_mv;
    uint32_t max_cell_voltage_mv;
    uint32_t max_charge_current_ma;
    uint32_t nominal_energy_wh;
    uint32_t max_charge_voltage_mv;
    int16_t  max_cell_temp_c;
    uint16_t initial_soc_permille;
} gbt_vehicle_params_t;

typedef struct {
    uint32_t voltage_mv;
    uint32_t current_ma;              /* magnitude */
    uint8_t  mode;                    /* SPN3074: 0x01 CV, 0x02 CC */
    uint16_t soc_permille;
} gbt_vehicle_demand_t;

/*
 * TX classes used by the TX gate (ARCHITECTURE.md §4.4). Each Lean message
 * has a fixed class:
 *   STOP  : BST, CST, BSD, CSD   (stop sequence, §11.3.8-11.4.2)
 *   ERROR : BEM, CEM             (timeout error messages, §11.5)
 *   NORMAL: every other message, including TP.CM / TP.DT
 */
#define GBT_NODE_TXCLASS_NORMAL 0u
#define GBT_NODE_TXCLASS_STOP   1u
#define GBT_NODE_TXCLASS_ERROR  2u

/*
 * ND-6 (OPEN): which classes may be sent after the output sink REFUSED a
 * safety command. No default is chosen in the architecture; a configuration
 * with GBT_NODE_FAILTX_UNSET is invalid (GBT_NODE_PHASE_CONFIG_ERROR), so the
 * decision cannot be made silently. Tests set it explicitly.
 *   OPTION_A: hold STOP and ERROR until a later command is accepted
 *   OPTION_B: allow STOP and ERROR (recorded as SAF-1 deviation)
 * NORMAL frames are never sent while the output state is FAILED.
 */
#define GBT_NODE_FAILTX_UNSET    0u
#define GBT_NODE_FAILTX_OPTION_A 1u
#define GBT_NODE_FAILTX_OPTION_B 2u

/* Result of gbt_node_tx_check(). */
#define GBT_NODE_TX_SEND    0u
#define GBT_NODE_TX_HOLD    1u
#define GBT_NODE_TX_DISCARD 2u

typedef struct {
    uint8_t  role;                            /* GBT_NODE_ROLE_* */
    const gbt_charger_params_t *charger;      /* required iff role == CHARGER */
    const gbt_vehicle_params_t *vehicle;      /* required iff role == VEHICLE */
    uint32_t t_handle_ms;                     /* K-6A, PROVISIONAL */
    uint32_t selfcheck_delay_ms;              /* S-8, PROVISIONAL */
    uint8_t  apply_fail_tx_policy;            /* GBT_NODE_FAILTX_*; ND-6 OPEN */
} gbt_node_config_t;

/* ---- Status and diagnostics ---- */
typedef struct {
    uint32_t rx_frames;
    uint32_t rx_ignored;          /* unknown or unexpected frames (§5.5) */
    uint32_t rx_invalid;          /* decode failures */
    uint32_t rx_overflow;         /* frames reported lost by the integration layer */
    uint32_t tx_frames;
    uint32_t outbox_overflow;
    uint32_t timeouts;            /* ERR-4: application timeouts */
    uint32_t transport_failures;  /* ERR-4: TP failures */
    uint32_t controller_failures; /* ERR-4 */
    uint32_t physical_failures;   /* ERR-4 */
    uint32_t output_failures;     /* sink refused an output command */
    uint32_t tx_discarded;        /* frames discarded by the TX gate or after send errors */
    uint32_t api_errors;          /* invalid arguments to gbt_node_* */
} gbt_node_diag_t;

typedef struct {
    uint8_t         role;
    uint8_t         phase;            /* GBT_NODE_PHASE_* */
    uint8_t         step;             /* protocol-private step code (C00.., V00..) */
    uint8_t         stop_reason;      /* GBT_STOP_REASON_* */
    uint8_t         output_state;     /* GBT_NODE_OUT_* */
    uint8_t         inhibit_latched;
    uint8_t         err_fields[4];    /* last BEM/CEM payload sent or received */
    gbt_node_diag_t diag;
} gbt_node_status_t;

/* ---- Node object (complete type for static allocation; members private) ---- */
/* Session data used by the state machines (private). */
typedef struct {
    uint8_t     tx_on[GBT_MSG_COUNT];      /* periodic transmission enabled */
    uint32_t    tx_due[GBT_MSG_COUNT];     /* next due time */
    uint8_t     crm_result;                /* SPN2560 being sent */
    uint8_t     bro_ready;                 /* SPN2829 being sent */
    uint8_t     cro_ready;                 /* SPN2830 being sent */
    uint8_t     ccs_permit;                /* SPN3929 being sent */
    uint8_t     stop_self;                 /* 1 if this node initiated the stop */
    uint8_t     stop_frames;               /* stop frames sent (vehicle, TM-06) */
    gbt_stop_t  stop_msg;                  /* BST or CST content */
    uint8_t     err_codes[GBT_CEM_FIELDS]; /* BEM or CEM content */
    uint32_t    handling_start_ms;         /* K-6A */
    uint8_t     selfcheck_pending;
    uint32_t    selfcheck_start_ms;
    gbt_bcp_t   bcp;
    uint8_t     bcp_valid;
    gbt_cml_t   cml;
    uint8_t     cml_valid;
    gbt_bcl_t   bcl;
    uint8_t     bsm_seen;
} gbt_node_session_t;

typedef struct {
    gbt_node_config_t     cfg;
    uint8_t               phase;
    uint8_t               step;
    uint8_t               connected;
    uint8_t               stop_request;
    uint8_t               stop_reason;
    uint32_t              now_ms;
    gbt_timer_t           timer[GBT_NODE_TIMER_SLOTS];
    gbt_tp_t              tp;
    gbt_outbox_t          outbox;
    gbt_output_cmd_t      out_cmd;
    uint32_t              out_accepted_seq;
    uint8_t               out_state;
    uint8_t               inhibit_latched;
    gbt_output_fb_t       feedback;
    gbt_vehicle_demand_t  demand;
    gbt_node_session_t    s;
    uint8_t               err_fields[4];
    gbt_node_diag_t       diag;
} gbt_node_t;

/* ---- Public API (declarations only) ---- */

/* Initialise; the configuration is copied. Output command starts disabled. */
void gbt_node_init(gbt_node_t *node, const gbt_node_config_t *config);

/* Process one received frame. Unknown frames are counted and ignored. */
void gbt_node_on_rx(gbt_node_t *node, const gbt_can_frame_t *frame, uint32_t now_ms);

/* Report frames lost before reaching the node (port RX queue or hardware FIFO). */
void gbt_node_on_rx_overflow(gbt_node_t *node, uint32_t lost_frames, uint32_t now_ms);

/* Periodic processing: timers, periodic messages, TP pacing. Call every <= 1 ms. */
void gbt_node_tick(gbt_node_t *node, uint32_t now_ms);

/*
 * Take the next frame to transmit, in FIFO order, subject to the TX gate
 * (ARCHITECTURE.md §4.4). Returns false if no frame is available or the head
 * frame must be held. Frames the gate discards are dropped internally and
 * counted. On true, ownership of the copied frame passes to the caller.
 */
bool gbt_node_pop_tx(gbt_node_t *node, gbt_can_frame_t *frame);

/* Re-evaluate the TX gate for a frame the caller already holds (pending). */
uint8_t gbt_node_tx_check(const gbt_node_t *node, const gbt_can_frame_t *frame);

/* Current software output command (copy). Compare seq to detect a change. */
void gbt_node_get_output(const gbt_node_t *node, gbt_output_cmd_t *cmd);

/*
 * Report the outcome of gbt_hal_output_apply() for the command with this seq.
 *   accepted = true  : the sink returned GBT_HAL_OK (software acceptance; not a
 *                      physical-output measurement). Output state -> ACCEPTED.
 *   accepted = false : any other status. Output state -> FAILED (ND-6 OPEN).
 * Must be called once, immediately after each gbt_hal_output_apply() of a new
 * command, and before the next gbt_node_pop_tx()/gbt_node_tx_check(). A seq
 * other than the latest command's seq is ignored and counted as an API error.
 * It confirms software acceptance only; it never changes the command itself.
 */
void gbt_node_confirm_output(gbt_node_t *node, uint32_t seq, bool accepted, uint32_t now_ms);

/* Latest measured output feedback from the application (charger). */
void gbt_node_set_feedback(gbt_node_t *node, const gbt_output_fb_t *fb);

/* Simulated physical connection / auxiliary power (PROT-1). */
void gbt_node_set_connected(gbt_node_t *node, uint8_t connected, uint32_t now_ms);

/* Local stop request (charger: CST path; vehicle: BST path). Role semantics: C-3. */
void gbt_node_request_stop(gbt_node_t *node, uint8_t reason, uint32_t now_ms);

/* Vehicle demand from the battery model (vehicle role). */
void gbt_node_set_vehicle_demand(gbt_node_t *node, const gbt_vehicle_demand_t *demand);

/* Start a new session after END or EXIT; returns false otherwise. */
bool gbt_node_restart_session(gbt_node_t *node, uint32_t now_ms);

/* Detach: latch inhibit and produce a final disabled output command (APPLY it). */
void gbt_node_shutdown(gbt_node_t *node, uint32_t now_ms);

/* Status and diagnostics snapshot. */
void gbt_node_get_status(const gbt_node_t *node, gbt_node_status_t *status);

#endif /* GBT_NODE_H */
