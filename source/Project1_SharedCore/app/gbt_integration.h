/*
 * app/gbt_integration.h - platform integration loop (ARCHITECTURE.md §4, AR-1 C-2).
 * Owns the ND-1 lifecycle (bind, claim, release, unbind) and every runtime HAL
 * call; the protocol node never sees the HAL. Same code for SIL and targets.
 */
#ifndef GBT_INTEGRATION_H
#define GBT_INTEGRATION_H

#include <stdint.h>
#include "hal/hal.h"
#include "protocol/gbt_node.h"

#define GBT_INTEG_RX_PER_STEP 16u

/*
 * STEP 07-C2.4 optional RAM event log (diagnostics only, Keil Watch readable).
 * Single writer: gbt_integ_step() in main context (no ISR access, no lock, no blocking).
 * Records, in processing order: TP frames popped from the HAL RX queue (before the node sees
 * them), TP frames accepted by the CAN driver (gbt_hal_can_send == OK), protocol step changes
 * and TP session ends. Non-TP frames are not logged. When all GBT_EVLOG_LEN entries are used
 * the log FREEZES (frozen = 1): entries are never overwritten, further events only increment
 * `dropped`. Entry fields by type (pgn = 24-bit PGN as in the TP frame / session):
 *   RX_TP_CM / TX_TP_CM : a = control byte (16 RTS, 17 CTS, 19 EOM, 255 abort), b = data[1], c = data[2]
 *                         (RTS/EOM: b,c = size low/high; CTS: b = packets granted, c = next sequence;
 *                          abort: b = reason); pgn = data[5..7]
 *   RX_TP_DT / TX_TP_DT : a = sequence number, b = data[1] (first payload byte), c = 0;
 *                         pgn = receive / send session PGN at that moment (0 if no session open)
 *   STEP                : a = new step, b = previous step, c = phase; pgn = 0
 *   TP_END              : a = GBT_TP_EV_* (1 RX done, 2 RX aborted, 4 TX done, 8 TX aborted),
 *                         b = DTs of that session (accepted / queued), c = packets expected; pgn = session PGN
 * t_ms = gbt_hal_now_ms() of the integration step (processing time, not bus time).
 */
#define GBT_EVLOG_LEN       128u
#define GBT_EV_RX_TP_CM     1u
#define GBT_EV_RX_TP_DT     2u
#define GBT_EV_TX_TP_CM     3u
#define GBT_EV_TX_TP_DT     4u
#define GBT_EV_STEP         5u
#define GBT_EV_TP_END       6u

typedef struct {
    uint32_t t_ms;
    uint32_t pgn;
    uint8_t  type;                     /* GBT_EV_* */
    uint8_t  a;
    uint8_t  b;
    uint8_t  c;
} gbt_evlog_entry_t;

typedef struct {
    uint32_t          count;           /* valid entries 0..GBT_EVLOG_LEN */
    uint32_t          dropped;         /* events not recorded because the log was full */
    uint32_t          frozen;          /* 1 once full */
    gbt_evlog_entry_t e[GBT_EVLOG_LEN];
} gbt_evlog_t;

/*
 * STEP 07 FI-04: output shutdown response-time measurement (diagnostics only, Keil Watch readable).
 * SOFTWARE COMMAND ACCEPTANCE ONLY: T1 is the moment the ND-1 output sink accepted the disabled
 * command (gbt_hal_output_apply == GBT_HAL_OK). It is NOT a measurement of physical power-stage
 * current decay (Project 1 has no power stage).
 * Clock: gbt_hal_now_ms() (1 ms resolution, wraps at 2^32; unsigned wrap-safe subtraction).
 *   T0 = start time of the integration pass PRECEDING the pass in which the disabled command was
 *        first delivered to the sink. Any event handled in pass P happened after pass P-1 started:
 *        an RX frame was not yet in the queue when pass P-1 drained it (GBT_HAL_E_EMPTY), a timer
 *        had not yet expired at pass P-1's tick, an API call between passes came after P-1 began.
 *        Commands first delivered outside gbt_integ_step() (gbt_integ_detach) use the start of
 *        the most recent pass. T0 is therefore never later than the event.
 *   T1 = gbt_hal_now_ms() read immediately after gbt_hal_output_apply() returned GBT_HAL_OK.
 *   elapsed = T1 - T0; true latency < elapsed + 1 ms, hence PASS only if elapsed <= 9 ms.
 * A measurement opens when a new command with enabled == 0 is delivered while the last command
 * ACCEPTED by the sink had enabled == 1, and closes only when a disabled command is accepted
 * (GBT_HAL_OK) AND the node confirmed exactly that sequence (out_accepted_seq == cmd.seq).
 * Sink refusals keep it open (the retry is included in the latency). An unchanged command never
 * gets a new sequence (gbt_safety.c new_command), so a shutdown is counted at most once.
 * Reference invalid (ref_flags != 0, verdict INVALID, never PASS) when: there is no previous pass
 * (NO_PREV), the previous pass hit GBT_INTEG_RX_PER_STEP (RX_LIMIT) or ended its RX loop with a
 * status other than GBT_HAL_E_EMPTY (RX_ERROR), an RX overflow was reported in the previous pass
 * or in the current pass before the measurement opened (OVERFLOW), or gbt_hal_can_get_status()
 * failed there so overflow could not be checked (STATUS_ERR).
 * Written only by the integration layer in main context; never read by the protocol logic.
 * Zeroed by gbt_integ_attach().
 */
#define GBT_FI04_LIMIT_MS          9u     /* PASS: elapsed_ms <= 9 (1 ms tick quantisation) */
#define GBT_FI04_VERDICT_NONE      0u
#define GBT_FI04_VERDICT_PASS      1u
#define GBT_FI04_VERDICT_FAIL      2u
#define GBT_FI04_VERDICT_INVALID   3u
#define GBT_FI04_REF_NO_PREV       0x01u
#define GBT_FI04_REF_RX_LIMIT      0x02u
#define GBT_FI04_REF_RX_ERROR      0x04u
#define GBT_FI04_REF_OVERFLOW      0x08u
#define GBT_FI04_REF_STATUS_ERR    0x10u

typedef struct {
    /* pass tracking (reference) */
    uint32_t         passes;              /* gbt_integ_step() passes started (saturating) */
    uint32_t         pass_start_ms;       /* start of the current / most recent pass */
    uint32_t         prev_pass_start_ms;  /* start of the pass before it (T0 candidate) */
    uint8_t          cur_ref_flags;       /* GBT_FI04_REF_* seen so far in the current pass */
    uint8_t          prev_ref_flags;      /* GBT_FI04_REF_* of the previous (complete) pass */
    uint8_t          in_pass;             /* 1 while inside gbt_integ_step() */
    uint8_t          acc_enabled;         /* enabled flag of the last command ACCEPTED by the sink */
    /* open measurement */
    uint8_t          open;                /* 1 = disabled command delivered, not yet accepted+confirmed */
    uint8_t          open_ref_flags;
    uint32_t         open_t0_ms;
    uint32_t         open_seq;            /* first disabled command sequence of this shutdown */
    /* last completed measurement */
    uint32_t         t0_ms;
    uint32_t         t1_ms;
    uint32_t         elapsed_ms;          /* t1_ms - t0_ms (wrap-safe) */
    uint32_t         first_seq;           /* sequence that opened the measurement */
    uint32_t         accept_seq;          /* sequence accepted by the sink (differs after a retry) */
    uint32_t         confirmed_seq;       /* node out_accepted_seq after gbt_node_confirm_output */
    gbt_hal_status_t apply_status;        /* gbt_hal_output_apply() result of the last delivery while open */
    uint8_t          ref_flags;           /* GBT_FI04_REF_* of the last completed measurement (0 = valid) */
    uint8_t          verdict;             /* GBT_FI04_VERDICT_* of the last completed measurement */
    /* totals since attach (saturating) */
    uint32_t         shutdowns;           /* accepted + confirmed enabled -> disabled transitions */
    uint32_t         pass_count;
    uint32_t         fail_count;
    uint32_t         invalid_count;
    uint32_t         max_valid_ms;        /* max elapsed_ms over measurements with a valid reference */
    uint32_t         refused;             /* deliveries refused by the sink while open */
    uint32_t         mismatch;            /* GBT_HAL_OK but the node did not confirm that sequence */
    uint32_t         aborted;             /* open measurement superseded by an accepted enabled command */
} gbt_fi04_t;

/* Pure bookkeeping (no HAL / node calls); used by gbt_integration.c, exposed for unit tests. */
void gbt_fi04_pass_begin(gbt_fi04_t *m, uint32_t now_ms);
void gbt_fi04_pass_note(gbt_fi04_t *m, uint8_t ref_flag);
void gbt_fi04_pass_end(gbt_fi04_t *m);
/* After every delivery of a NEW command sequence to the sink: st = gbt_hal_output_apply() result,
 * confirmed_seq = node out_accepted_seq after gbt_node_confirm_output(), t1_ms = clock read right
 * after gbt_hal_output_apply() returned. */
void gbt_fi04_on_apply(gbt_fi04_t *m, const gbt_output_cmd_t *cmd, gbt_hal_status_t st,
                       uint32_t confirmed_seq, uint32_t t1_ms);

typedef struct {
    gbt_node_t      *node;
    gbt_hal_t       *hal;
    gbt_can_frame_t  pending;          /* single pending TX frame (AR-4) */
    uint8_t          has_pending;
    uint32_t         applied_seq;      /* last output seq delivered to the sink */
    uint32_t         last_ovf_total;
    uint32_t         tx_dropped;       /* E_BUS_OFF / E_IO / ... (ND-3 PROVISIONAL) */
    uint32_t         tx_full_retries;
    /* Optional app-level fault injection: return 0 to drop the frame before
     * gbt_hal_can_send (test/SIL use only; NULL in normal operation). */
    uint8_t        (*tx_filter)(void *ctx, const gbt_can_frame_t *f);
    void            *tx_filter_ctx;
    uint32_t         tx_filtered;
    /* ---- STEP 07-C2 diagnostics: written in gbt_integ_step()/APPLY (main context
     * only), never read by the integration logic. Zeroed by gbt_integ_attach(). ---- */
    gbt_can_frame_t  last_rx;          /* last frame popped from the HAL RX queue (before gbt_node_on_rx) */
    uint8_t          last_rx_valid;    /* 0 = nothing popped since attach; 1 = last_rx holds a popped frame */
    uint32_t         rx_seq;           /* frames popped from the HAL RX queue since attach (saturating) */
    gbt_output_cmd_t last_cmd;         /* last command delivered to the output sink (APPLY) */
    gbt_hal_status_t last_apply_status;/* sink result for last_cmd (GBT_HAL_OK = accepted) */
    uint32_t         output_shutdowns; /* delivered commands with enabled 1 -> 0 (saturating) */
    /* Driver acceptance (event level 3): gbt_hal_can_send() returned GBT_HAL_OK, i.e. the HAL
     * took the frame into a TX mailbox / TX FIFO. NOT proof of bus transmission. Saturating. */
    uint32_t         tx_hal_accepted;       /* all frames */
    uint32_t         tx_hal_accepted_tp_cm; /* of which TP.CM (PDU format = node TP pf_cm) */
    uint32_t         tx_hal_accepted_tp_dt; /* of which TP.DT (PDU format = node TP pf_dt) */
    /* ---- STEP 07-C2.4 diagnostics (same rules: main context only, never read by the logic) ----
     * Completed TP sessions per message, detected exactly once per completion from the node's
     * TP statistics after every gbt_node_on_rx / gbt_node_tick (rx: last DT accepted; tx: EOM
     * accepted). Index: 0 BRM, 1 BCP, 2 BCS, 3 any other PGN. Saturating. */
    uint32_t         tp_rx_complete_msg[4];
    uint32_t         tp_tx_complete_msg[4];
    uint32_t         seen_rx_complete;      /* last observed tp.stats.rx_complete */
    uint32_t         seen_tx_complete;      /* last observed tp.stats.tx_complete */
    uint32_t         seen_sessions_ended;   /* last observed tp.stats.sessions_ended */
    uint8_t          seen_step;             /* last observed node step (event log) */
    uint32_t         step_changes;          /* protocol step changes observed (saturating) */
    gbt_evlog_t     *evlog;                 /* optional event log; NULL = disabled */
    gbt_fi04_t       fi04;                  /* STEP 07 FI-04 shutdown response time (diagnostics) */
} gbt_integ_t;

/* bind sink -> claim -> gbt_node_init -> apply initial disabled command. */
gbt_hal_status_t gbt_integ_attach(gbt_integ_t *ig, gbt_node_t *node, gbt_hal_t *hal,
                                  const gbt_node_config_t *cfg, const gbt_output_sink_t *sink);
/* One main-loop iteration: overflow, RX (bounded), tick, APPLY, SEND. */
void gbt_integ_step(gbt_integ_t *ig);
/* STEP 07-C2.4: attach (log != NULL, cleared here) or detach (NULL) the optional event log.
 * Call after gbt_integ_attach(); gbt_integ_attach() resets it to NULL. */
void gbt_integ_set_evlog(gbt_integ_t *ig, gbt_evlog_t *log);
/* gbt_node_shutdown -> apply -> release -> unbind. */
gbt_hal_status_t gbt_integ_detach(gbt_integ_t *ig);

#endif /* GBT_INTEGRATION_H */
