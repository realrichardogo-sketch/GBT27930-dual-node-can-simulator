/* tests/support/fake_hal.c - Fake HAL implementing hal/hal.h, ND-1 (see fake_hal.h). */
#include "tests/support/fake_hal.h"

#include <stddef.h>
#include <string.h>

static void sat_inc(uint32_t *c)
{
    if (*c != 0xFFFFFFFFu) {
        (*c)++;
    }
}

static void rxq_push(gbt_hal_t *h, const gbt_can_frame_t *f)
{
    if (h->rx_count >= GBT_FAKE_RXQ_LEN) {
        sat_inc(&h->status.rx_overflow_sw);   /* software drop */
        return;
    }
    h->rxq[h->rx_head] = *f;
    h->rx_head = (h->rx_head + 1U) % GBT_FAKE_RXQ_LEN;
    h->rx_count++;
}

/* ---- fake-specific API ---- */
gbt_hal_status_t gbt_fake_hal_init(gbt_hal_t *h, gbt_sim_bus_t *bus, uint8_t node_index,
                                   const uint32_t *virtual_now_ms)
{
    if (h == NULL || bus == NULL || virtual_now_ms == NULL || node_index > 1U) {
        return GBT_HAL_E_PARAM;
    }
    if (h->lifecycle != GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_STATE;
    }
    memset(h, 0, sizeof *h);
    h->bus = bus;
    h->node_index = node_index;
    h->virtual_now_ms = virtual_now_ms;
    h->inject_tx_status = (uint8_t)GBT_HAL_OK;
    h->status.state = (uint8_t)GBT_CAN_STATE_ERROR_ACTIVE;
    h->status.tec_rec_valid = 1U;
    bus->node[node_index] = h;
    h->lifecycle = (uint8_t)GBT_HAL_LC_READY;
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_fake_hal_deinit(gbt_hal_t *h)
{
    if (h == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U || h->lifecycle != GBT_HAL_LC_READY) {
        return GBT_HAL_E_STATE;          /* only READY -> UNINIT */
    }
    if (h->bus != NULL && h->bus->node[h->node_index] == h) {
        h->bus->node[h->node_index] = NULL;
    }
    h->lifecycle = (uint8_t)GBT_HAL_LC_UNINIT;
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_fake_hal_inject_rx(gbt_hal_t *h, const gbt_can_frame_t *f)
{
    if (h == NULL || gbt_can_frame_is_valid(f) == 0U) {
        return GBT_HAL_E_PARAM;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_NOT_READY;
    }
    rxq_push(h, f);
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_fake_hal_inject_overflow(gbt_hal_t *h, uint8_t hardware)
{
    if (h == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (hardware != 0U) {
        sat_inc(&h->status.rx_overflow_hw);
    } else {
        sat_inc(&h->status.rx_overflow_sw);
    }
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_fake_hal_set_tx_status(gbt_hal_t *h, gbt_hal_status_t st)
{
    if (h == NULL) {
        return GBT_HAL_E_PARAM;
    }
    h->inject_tx_status = (uint8_t)st;
    return GBT_HAL_OK;
}

/* ---- hal/hal.h lifecycle (ND-1) ---- */
gbt_hal_status_t gbt_hal_bind_output_sink(gbt_hal_t *h, const gbt_output_sink_t *sink)
{
    if (h == NULL || sink == NULL || sink->apply == NULL || sink->feedback == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_NOT_READY;
    }
    if (h->lifecycle != GBT_HAL_LC_READY) {
        return GBT_HAL_E_STATE;          /* bind in BOUND or CLAIMED is rejected */
    }
    h->sink = *sink;                     /* the struct is copied */
    h->lifecycle = (uint8_t)GBT_HAL_LC_BOUND;
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_hal_unbind_output_sink(gbt_hal_t *h)
{
    if (h == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U || h->lifecycle != GBT_HAL_LC_BOUND) {
        return GBT_HAL_E_STATE;          /* unbind in CLAIMED is rejected */
    }
    memset(&h->sink, 0, sizeof h->sink);
    h->lifecycle = (uint8_t)GBT_HAL_LC_READY;
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_hal_claim(gbt_hal_t *h)
{
    if (h == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT || h->lifecycle == GBT_HAL_LC_READY) {
        return GBT_HAL_E_NOT_READY;      /* not initialised, or no sink bound */
    }
    if (h->lifecycle != GBT_HAL_LC_BOUND) {
        return GBT_HAL_E_STATE;          /* already claimed */
    }
    h->lifecycle = (uint8_t)GBT_HAL_LC_CLAIMED;
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_hal_release(gbt_hal_t *h)
{
    if (h == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U || h->lifecycle != GBT_HAL_LC_CLAIMED) {
        return GBT_HAL_E_STATE;
    }
    h->lifecycle = (uint8_t)GBT_HAL_LC_BOUND;
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_hal_output_apply(gbt_hal_t *h, const gbt_output_cmd_t *cmd)
{
    gbt_hal_status_t st;

    if (h == NULL || cmd == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U || h->lifecycle != GBT_HAL_LC_CLAIMED) {
        return GBT_HAL_E_STATE;
    }
    if (h->sink.apply == NULL) {
        return GBT_HAL_E_NOT_READY;
    }
    h->in_sink = 1U;
    st = h->sink.apply(h->sink.ctx, cmd);
    h->in_sink = 0U;
    if (st == GBT_HAL_OK) {               /* acceptance bookkeeping on OK only */
        h->accepted_seq = cmd->seq;
        h->accept_time_ms = gbt_hal_now_ms(h);
    }
    return st;
}

gbt_hal_status_t gbt_hal_output_feedback(gbt_hal_t *h, gbt_output_fb_t *fb)
{
    gbt_hal_status_t st;

    if (h == NULL || fb == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U || h->lifecycle != GBT_HAL_LC_CLAIMED) {
        return GBT_HAL_E_STATE;
    }
    if (h->sink.feedback == NULL) {
        return GBT_HAL_E_NOT_READY;
    }
    h->in_sink = 1U;
    st = h->sink.feedback(h->sink.ctx, fb);
    h->in_sink = 0U;
    return st;
}

/* ---- hal/hal.h runtime ---- */
gbt_hal_status_t gbt_hal_can_send(gbt_hal_t *h, const gbt_can_frame_t *f)
{
    gbt_hal_t *peer;

    if (h == NULL || gbt_can_frame_is_valid(f) == 0U) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_NOT_READY;
    }
    if (h->inject_tx_status != (uint8_t)GBT_HAL_OK) {
        sat_inc(&h->status.tx_error);
        return (gbt_hal_status_t)h->inject_tx_status;   /* frame NOT accepted */
    }
    peer = (h->bus != NULL) ? h->bus->node[(h->node_index == 0U) ? 1U : 0U] : NULL;
    if (h->bus != NULL && h->bus->filter != NULL
        && h->bus->filter(h->bus->filter_ctx, h->node_index, f) == 0U) {
        sat_inc(&h->bus->frames_dropped);       /* injected loss on the wire */
        return GBT_HAL_OK;
    }
    if (peer != NULL && peer->lifecycle != GBT_HAL_LC_UNINIT) {
        rxq_push(peer, f);
        sat_inc(&h->bus->frames_delivered);
    }
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_hal_can_receive(gbt_hal_t *h, gbt_can_frame_t *out)
{
    if (h == NULL || out == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_NOT_READY;
    }
    if (h->rx_count == 0U) {
        return GBT_HAL_E_EMPTY;
    }
    *out = h->rxq[h->rx_tail];
    h->rx_tail = (h->rx_tail + 1U) % GBT_FAKE_RXQ_LEN;
    h->rx_count--;
    return GBT_HAL_OK;
}

uint32_t gbt_hal_now_ms(gbt_hal_t *h)
{
    if (h == NULL || h->lifecycle == GBT_HAL_LC_UNINIT || h->virtual_now_ms == NULL) {
        return 0U;
    }
    return *h->virtual_now_ms;
}

gbt_hal_status_t gbt_hal_can_get_status(gbt_hal_t *h, gbt_can_status_t *s)
{
    if (h == NULL || s == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_NOT_READY;
    }
    *s = h->status;
    return GBT_HAL_OK;
}

void gbt_hal_event(gbt_hal_t *h, uint16_t code, int32_t a, int32_t b)
{
    (void)a;
    (void)b;
    if (h == NULL || h->lifecycle == GBT_HAL_LC_UNINIT) {
        return;
    }
    sat_inc(&h->event_count);
    h->last_event_code = code;
}
