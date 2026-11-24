/*
 * protocol/gbt_tp.h - connection-mode transport protocol (RTS/CTS) for
 * BRM (49 B), BCP (13 B) and BCS (9 B). Pure C99; frames go to a node outbox.
 *
 * Normative (GB/T 27930-2023 §7.5, VERIFIED_GBT): TP for 9..1785-byte PGs,
 * connection management per SAE J1939-21:2006 §5.4.7 / §5.10, 10 ms between
 * data frames, message period = whole-packet period.
 *
 * Everything taken from SAE J1939-21 (PGNs, control bytes, layouts, sequence
 * numbering, timers, abort reasons, priority) is NOT verified against the 2006
 * edition (K-4 OPEN). Those values live in gbt_tp_params_t; defaults are
 * defined by gbt_tp_default_params() and registered in
 * docs/PROVISIONAL_PARAMETERS.md (P-K4-xx). Replace them there and only there.
 *
 * Lean scope: happy path, sequence-gap abort (duplicates are treated as gaps),
 * bounded waits on both sides (K-4.b), one session per direction, 64-byte
 * receive buffer (TP-4). No BAM, no retransmission requests.
 */
#ifndef GBT_TP_H
#define GBT_TP_H

#include <stdint.h>
#include "common/gbt_types.h"
#include "protocol/gbt_outbox.h"
#include "protocol/gbt_timer.h"

#define GBT_TP_MAX_LEN       64u   /* TP-4 project buffer (PROJECT_DEFINED) */
#define GBT_TP_BYTES_PER_DT  7u    /* P-K4-08 PROVISIONAL */

typedef struct {
    uint8_t  pf_cm;              /* P-K4-01: TP.CM PDU format (PGN 0x00EC00) */
    uint8_t  pf_dt;              /* P-K4-02: TP.DT PDU format (PGN 0x00EB00) */
    uint8_t  priority;           /* P-K4-03 */
    uint8_t  ctrl_rts;           /* P-K4-04 */
    uint8_t  ctrl_cts;           /* P-K4-04 */
    uint8_t  ctrl_eom;           /* P-K4-04 */
    uint8_t  ctrl_abort;         /* P-K4-04 */
    uint8_t  first_seq;          /* P-K4-09 */
    uint8_t  abort_busy;         /* P-K4-10: already in a session */
    uint8_t  abort_resources;    /* P-K4-10: request not acceptable */
    uint8_t  abort_timeout;      /* P-K4-10 */
    uint8_t  abort_seq;          /* P-K4-10: sequence error */
    uint32_t dt_interval_ms;     /* GB/T §7.5: 10 ms (VERIFIED_GBT) */
    uint32_t t1_ms;              /* P-K4-05: receiver, gap between DTs */
    uint32_t t2_ms;              /* P-K4-05: receiver, CTS sent -> first DT */
    uint32_t t3_ms;              /* P-K4-05: sender, RTS or last DT of window -> CTS/EOM */
    uint32_t t4_ms;              /* P-K4-05: sender, CTS(0) hold -> next CTS */
} gbt_tp_params_t;

/* Fills *p with the registered PROVISIONAL defaults. */
void gbt_tp_default_params(gbt_tp_params_t *p);

#define GBT_TP_IDLE        0u
#define GBT_TP_RX_DATA     1u   /* receiving DTs */
#define GBT_TP_TX_WAIT_CTS 2u   /* RTS or window sent, waiting for CTS/EOM */
#define GBT_TP_TX_SENDING  3u   /* sending DTs at dt_interval */
#define GBT_TP_TX_HOLD     4u   /* CTS(0) received */

typedef struct {
    uint8_t     state;
    uint32_t    pgn;
    uint16_t    len;
    uint8_t     packets;
    uint8_t     next_seq;     /* RX: expected; TX: next to send */
    uint8_t     window_end;   /* TX: last seq of current CTS window */
    uint32_t    last_dt_ms;
    gbt_timer_t wait;
    uint8_t     buf[GBT_TP_MAX_LEN];
} gbt_tp_session_t;

/*
 * TP event statistics (STEP 07-C2 diagnostic instrumentation).
 *
 * Pure accounting: written only inside gbt_tp.c (main context), never read by
 * the TP logic, so they cannot influence any protocol decision. Zeroed by
 * gbt_tp_init() (and therefore by gbt_node_init / gbt_node_restart_session).
 * Every counter saturates at 0xFFFFFFFF.
 *
 * "queued" means gbt_outbox_push() returned GBT_RING_OK: the frame entered the
 * node outbox. It does NOT prove that the frame was sent on the CAN bus (that
 * is decided later by the TX gate and the HAL); a failed push is counted in
 * outbox_full only, never in a *_tx statistic.
 * Exact increment points: docs/TP_COUNTER_SEMANTICS.md.
 */
typedef struct {
    /* control frames */
    uint32_t rts_tx;               /* RTS queued by gbt_tp_tx_start() */
    uint32_t rts_rx;               /* RTS accepted: a receive session was opened */
    uint32_t cts_tx;               /* CTS queued (session-opening CTS and window CTS) */
    uint32_t cts_rx;               /* CTS accepted by the open send session (grant or CTS(0) hold) */
    uint32_t eom_tx;               /* EOM queued after the last DT was accepted */
    uint32_t eom_rx;               /* EOM accepted: all packets sent, waiting, same PGN */
    /* data frames */
    uint32_t dt_tx;                /* DT queued */
    uint32_t dt_rx_ok;             /* DT with the expected sequence number accepted and stored */
    /* sessions */
    uint32_t tx_started;           /* send session opened (gbt_tp_tx_start returned 1) */
    uint32_t rx_started;           /* receive session opened (same event as rts_rx) */
    uint32_t tx_complete;          /* send session ended by an accepted EOM (GBT_TP_EV_TX_COMPLETE) */
    uint32_t rx_complete;          /* receive session ended by its last DT (GBT_TP_EV_RX_COMPLETE) */
    /* failures */
    uint32_t abort_tx;             /* Connection Abort frames queued by this node (any reason) */
    uint32_t abort_rx;             /* received Connection Aborts, counted per local session they ended */
    uint32_t timeout_tx;           /* send sessions ended by T3 / T4 expiry */
    uint32_t timeout_rx;           /* receive sessions ended by T1 / T2 expiry */
    /* last events */
    uint32_t last_tx_complete_pgn; /* PGN of the last completed send session (0 = none yet) */
    uint32_t last_rx_complete_pgn; /* PGN of the last completed receive session (0 = none yet) */
    uint16_t last_tx_complete_len;
    uint16_t last_rx_complete_len;
    uint8_t  last_abort_reason_tx; /* reason byte of the last abort queued (0 = none yet) */
    uint8_t  last_abort_reason_rx; /* reason byte of the last received abort that ended a session */
    uint8_t  last_end_ev;          /* GBT_TP_EV_* flag of the most recent session end (0 = none yet) */
    uint8_t  reserved;
    /* ---- STEP 07-C2.4: per-session accounting and timing (same rules as above: written next
     * to existing transitions, never read by the TP logic). Times are the now_ms values passed
     * to gbt_tp_* by the caller (main-loop processing time, not CAN bus time). ---- */
    uint32_t sessions_ended;       /* session ends of either direction (each write of last_end_ev) */
    uint32_t tx_sess_start_ms;     /* now_ms when the current / last send session was opened */
    uint32_t rx_sess_start_ms;     /* now_ms when the current / last receive session was opened */
    uint32_t tx_sess_dt;           /* DTs queued in the current / last send session */
    uint32_t rx_sess_dt;           /* DTs accepted in the current / last receive session */
    uint32_t rx_last_dt_ms;        /* now_ms of the last accepted DT (interval reference) */
    uint32_t last_tx_duration_ms;  /* last COMPLETED send session: open (RTS queued) -> EOM accepted */
    uint32_t last_rx_duration_ms;  /* last COMPLETED receive session: RTS accepted -> last DT accepted */
    uint32_t max_tx_duration_ms;   /* maximum of last_tx_duration_ms */
    uint32_t max_rx_duration_ms;   /* maximum of last_rx_duration_ms */
    uint32_t max_tx_dt_interval_ms;/* largest gap between two consecutive DTs queued in one send session */
    uint32_t max_rx_dt_interval_ms;/* largest gap between two consecutive DTs accepted in one receive session */
} gbt_tp_stats_t;

/*
 * STEP 07-C2.4 diagnostic payload snapshot of the PGN GBT_TP_SNAP_PGN (BRM, 49 bytes;
 * PGN 512 in protocol/gbt_id.c). Never read by the TP logic.
 *   send side:    copied at gbt_tp_tx_start() of every BRM session until one BRM send session
 *                 has completed (EOM accepted); state then becomes FROZEN and the copy is never
 *                 written again (later BCP/BCS sessions use other PGNs and cannot touch it).
 *   receive side: copied when the first BRM receive session completes (last DT accepted);
 *                 state FROZEN at once.
 */
#define GBT_TP_SNAP_PGN        512u  /* BRM; asserted equal to gbt_msg_def(GBT_MSG_BRM)->pgn in test_tp */
#define GBT_TP_SNAP_NONE       0u
#define GBT_TP_SNAP_CAPTURED   1u    /* send side only: copied, that session not completed yet */
#define GBT_TP_SNAP_FROZEN     2u    /* copied from a completed session; never written again */
typedef struct {
    uint8_t  state;                /* GBT_TP_SNAP_* */
    uint8_t  reserved;
    uint16_t len;                  /* payload length (BRM: 49) */
    uint32_t pgn;
    uint32_t captures;             /* number of copies made (send side: BRM session starts until frozen) */
    uint8_t  buf[GBT_TP_MAX_LEN];  /* bytes 0..len-1 valid */
} gbt_tp_snap_t;

typedef struct {
    gbt_tp_params_t  prm;
    uint8_t          own_sa;
    uint8_t          peer_sa;
    gbt_tp_session_t rx;
    gbt_tp_session_t tx;
    uint32_t         aborts_sent;
    uint32_t         aborts_received;
    uint32_t         outbox_full;
    gbt_tp_stats_t  stats;         /* STEP 07-C2 diagnostics; read-only for callers */
    gbt_tp_snap_t   snap_tx;       /* STEP 07-C2.4 BRM send snapshot; read-only for callers */
    gbt_tp_snap_t   snap_rx;       /* STEP 07-C2.4 BRM receive snapshot; read-only for callers */
} gbt_tp_t;

/* Event bit flags returned by gbt_tp_on_frame / gbt_tp_tick (may be combined) */
#define GBT_TP_EV_NONE        0x00u
#define GBT_TP_EV_RX_COMPLETE 0x01u   /* rx.pgn, rx.len, rx.buf hold the payload */
#define GBT_TP_EV_RX_ABORTED  0x02u
#define GBT_TP_EV_TX_COMPLETE 0x04u
#define GBT_TP_EV_TX_ABORTED  0x08u
#define GBT_TP_EV_NOT_TP      0x10u   /* frame is not TP.CM/TP.DT for this node */
/* gbt_tp_stats_t.last_end_ev holds exactly one of RX_COMPLETE, RX_ABORTED,
 * TX_COMPLETE, TX_ABORTED (or 0 before the first session end); every write also increments
 * gbt_tp_stats_t.sessions_ended (STEP 07-C2.4). */

void    gbt_tp_init(gbt_tp_t *tp, uint8_t own_sa, uint8_t peer_sa, const gbt_tp_params_t *prm);
/* Start sending a payload (9..64 bytes). Returns 0 if busy or invalid. */
uint8_t gbt_tp_tx_start(gbt_tp_t *tp, uint32_t pgn, const uint8_t *data, uint16_t len,
                        uint32_t now_ms, gbt_outbox_t *ob);
uint8_t gbt_tp_tx_busy(const gbt_tp_t *tp);
/* Accepted receive PGNs/lengths are checked by the caller via this hook. */
typedef uint8_t (*gbt_tp_accept_fn)(uint32_t pgn, uint16_t len);
uint8_t gbt_tp_on_frame(gbt_tp_t *tp, const gbt_can_frame_t *f, uint32_t now_ms,
                        gbt_outbox_t *ob, gbt_tp_accept_fn accept);
uint8_t gbt_tp_tick(gbt_tp_t *tp, uint32_t now_ms, gbt_outbox_t *ob);

#endif /* GBT_TP_H */
